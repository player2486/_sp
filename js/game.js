(function () {
  'use strict';

  const CANVAS_W = 1400;
  const CANVAS_H = 600;
  const GROUND_Y = 550;
  const SLING_X = 200;
  const SLING_Y = 435;
  const SLING_FORK_L = { x: SLING_X - 14, y: SLING_Y - 42 };
  const SLING_FORK_R = { x: SLING_X + 14, y: SLING_Y - 42 };
  const MAX_DRAG = 160;
  const LAUNCH_FORCE = 0.007;
  const SETTLE_SPEED = 0.35;
  const SETTLE_FRAMES = 100;
  const BIRD_RADIUS = 15;

  const { Engine, World, Bodies, Body, Events, Composite } = Matter;

  const G = {
    engine: null,
    canvas: null,
    ctx: null,
    scale: 1,

    birds: [],
    pigs: [],
    blocks: [],
    particles: [],
    ground: null,
    walls: [],

    currentBird: null,
    birdUsed: 0,
    totalBirds: 0,

    isDragging: false,
    dragEnd: null,

    phase: 'ready',
    settleTimer: 0,
    score: 0,
    levelIdx: 0,
  };

  /* ============ INIT ============ */

  function init() {
    G.canvas = document.getElementById('gameCanvas');
    G.canvas.width = CANVAS_W;
    G.canvas.height = CANVAS_H;
    G.ctx = G.canvas.getContext('2d');
    resize();
    window.addEventListener('resize', resize);

    G.engine = Engine.create({ gravity: { x: 0, y: 1.5 } });

    createBoundaries();
    setupInput();
    setupCollisions();

    loadLevel(0);
    loop();
  }

  function resize() {
    const cw = window.innerWidth - 24;
    const ch = window.innerHeight - 24;
    const s = Math.min(cw / CANVAS_W, ch / CANVAS_H, 1.3);
    G.scale = s;
    G.canvas.style.width = (CANVAS_W * s) + 'px';
    G.canvas.style.height = (CANVAS_H * s) + 'px';
  }

  /* ============ BOUNDARIES ============ */

  function createBoundaries() {
    const opts = { isStatic: true, friction: 1, restitution: 0.05 };
    G.ground = Bodies.rectangle(CANVAS_W / 2, GROUND_Y + 30, CANVAS_W + 400, 60, opts);
    G.walls = [
      Bodies.rectangle(-30, CANVAS_H / 2, 60, CANVAS_H * 2, opts),
      Bodies.rectangle(CANVAS_W + 30, CANVAS_H / 2, 60, CANVAS_H * 2, opts),
    ];
    World.add(G.engine.world, [G.ground, ...G.walls]);
  }

  /* ============ ENTITIES ============ */

  function makeBird(x, y) {
    const b = Bodies.circle(x, y, BIRD_RADIUS, {
      restitution: 0.25, friction: 0.4, density: 0.0045, label: 'bird',
    });
    b.data = { kind: 'bird' };
    b._isBird = true;
    return b;
  }

  function makePig(x, y, size) {
    const r = size === 'large' ? 24 : size === 'medium' ? 18 : 14;
    const hp = size === 'large' ? 3 : size === 'medium' ? 2 : 1;
    const p = Bodies.circle(x, y, r, {
      restitution: 0.15, friction: 0.8, density: 0.0025, label: 'pig',
    });
    p.data = { kind: 'pig', size, hp, maxHp: hp, r };
    return p;
  }

  function makeBlock(x, y, w, h, type, angle) {
    const props = {
      wood: { color: '#C4A265', hp: 2, density: 0.0035, restitution: 0.08 },
      stone: { color: '#909090', hp: 5, density: 0.006, restitution: 0.03 },
      glass: { color: '#A8D8EA', hp: 1, density: 0.0012, restitution: 0.25 },
    };
    const p = props[type] || props.wood;
    const b = Bodies.rectangle(x, y, w, h, {
      restitution: p.restitution, friction: 0.5, density: p.density,
      angle: angle || 0, label: 'block',
    });
    b.data = { kind: 'block', type, hp: p.hp, maxHp: p.hp, w, h, color: p.color };
    b._color = p.color;
    return b;
  }

  /* ============ LEVELS ============ */

  function loadLevel(idx) {
    clearWorld();
    const lv = LEVELS[idx];
    G.levelIdx = idx;
    G.totalBirds = lv.birds;
    G.birdUsed = 0;
    G.score = 0;
    G.phase = 'ready';
    G.settleTimer = 0;
    G.particles = [];
    G.pigs = [];
    G.blocks = [];

    for (const pd of lv.pigs) {
      const p = makePig(pd.x, pd.y, pd.size);
      G.pigs.push(p);
      World.add(G.engine.world, p);
    }
    for (const bd of lv.blocks) {
      const b = makeBlock(bd.x, bd.y, bd.w, bd.h, bd.type, bd.angle || 0);
      G.blocks.push(b);
      World.add(G.engine.world, b);
    }

    spawnBird();
    refreshUI();
    document.getElementById('level-info').textContent = `第 ${idx + 1} 關`;
    hideOverlay();
  }

  function clearWorld() {
    const keep = [G.ground, ...G.walls];
    const all = Composite.allBodies(G.engine.world);
    for (const b of all) {
      if (!keep.includes(b)) World.remove(G.engine.world, b);
    }
  }

  function spawnBird() {
    if (G.currentBird) World.remove(G.engine.world, G.currentBird);
    if (G.birdUsed >= G.totalBirds) { G.currentBird = null; return; }
    const b = makeBird(SLING_X, SLING_Y);
    Body.setStatic(b, true);
    G.currentBird = b;
    World.add(G.engine.world, b);
    G.birdUsed++;
    G.phase = 'ready';
    refreshUI();
  }

  /* ============ INPUT ============ */

  function setupInput() {
    const cvs = G.canvas;

    function pos(e) {
      const rect = cvs.getBoundingClientRect();
      const cx = (e.clientX || (e.touches && e.touches[0].clientX)) - rect.left;
      const cy = (e.clientY || (e.touches && e.touches[0].clientY)) - rect.top;
      return { x: cx / G.scale, y: cy / G.scale };
    }

    function down(e) {
      e.preventDefault();
      const p = pos(e);
      if (G.phase !== 'ready' || !G.currentBird) return;
      const d = dist(p, { x: SLING_X, y: SLING_Y });
      if (d < 50) {
        G.isDragging = true;
        G.dragEnd = { x: p.x, y: p.y };
        G.phase = 'aiming';
      }
    }

    function move(e) {
      e.preventDefault();
      if (!G.isDragging) return;
      G.dragEnd = pos(e);
    }

    function up(e) {
      e.preventDefault();
      if (!G.isDragging) return;
      G.isDragging = false;
      if (G.phase === 'aiming') launch();
    }

    cvs.addEventListener('mousedown', down);
    cvs.addEventListener('mousemove', move);
    cvs.addEventListener('mouseup', up);
    cvs.addEventListener('mouseleave', (e) => { if (G.isDragging) { G.isDragging = false; launch(); } });
    cvs.addEventListener('touchstart', down, { passive: false });
    cvs.addEventListener('touchmove', move, { passive: false });
    cvs.addEventListener('touchend', up, { passive: false });

    document.getElementById('btn-reset').addEventListener('click', () => loadLevel(G.levelIdx));
  }

  function dist(a, b) {
    return Math.sqrt((a.x - b.x) ** 2 + (a.y - b.y) ** 2);
  }

  /* ============ LAUNCH ============ */

  function launch() {
    if (!G.currentBird) return;
    const dx = SLING_X - G.dragEnd.x;
    const dy = SLING_Y - G.dragEnd.y;
    const d = Math.sqrt(dx * dx + dy * dy);
    if (d < 15) { G.phase = 'ready'; return; }

    const capped = Math.min(d, MAX_DRAG);
    const r = capped / d;
    Body.setStatic(G.currentBird, false);
    Body.setVelocity(G.currentBird, {
      x: dx * r * LAUNCH_FORCE,
      y: dy * r * LAUNCH_FORCE,
    });
    G.phase = 'flying';
    G.settleTimer = 0;
    refreshUI();
  }

  /* ============ COLLISIONS ============ */

  function setupCollisions() {
    Events.on(G.engine, 'collisionStart', (ev) => {
      for (const pair of ev.pairs) {
        const a = pair.bodyA;
        const b = pair.bodyB;
        if (a.isStatic && b.isStatic) continue;

        const rv = {
          x: a.velocity.x - b.velocity.x,
          y: a.velocity.y - b.velocity.y,
        };
        const speed = Math.sqrt(rv.x * rv.x + rv.y * rv.y);
        const dmg = speed * 2.0;

        hurt(a, dmg);
        hurt(b, dmg);
      }
    });
  }

  function hurt(body, dmg) {
    if (!body.data || !body.data.hp || body.data.kind === 'bird') return;
    body.data.hp -= dmg;
    if (body.data.hp <= 0) destroy(body);
  }

  function destroy(body) {
    const color = body.data.color || '#888';
    const count = body.data.kind === 'pig' ? 12 : 8;
    spawnParticles(body.position.x, body.position.y, color, count);

    if (body.data.kind === 'pig') {
      G.score += 5000;
      const idx = G.pigs.indexOf(body);
      if (idx > -1) G.pigs.splice(idx, 1);
    } else if (body.data.kind === 'block') {
      const pts = body.data.type === 'stone' ? 1000 : body.data.type === 'wood' ? 500 : 200;
      G.score += pts;
      const idx = G.blocks.indexOf(body);
      if (idx > -1) G.blocks.splice(idx, 1);
    }
    World.remove(G.engine.world, body);
    refreshUI();
  }

  /* ============ PARTICLES ============ */

  function spawnParticles(x, y, color, n) {
    for (let i = 0; i < n; i++) {
      const angle = Math.random() * Math.PI * 2;
      const spd = 2 + Math.random() * 5;
      G.particles.push({
        x, y,
        vx: Math.cos(angle) * spd,
        vy: Math.sin(angle) * spd - 3,
        size: 2 + Math.random() * 6,
        color,
        life: 1,
        decay: 0.012 + Math.random() * 0.025,
      });
    }
  }

  function updateParticles() {
    for (let i = G.particles.length - 1; i >= 0; i--) {
      const p = G.particles[i];
      p.x += p.vx;
      p.y += p.vy;
      p.vy += 0.18;
      p.life -= p.decay;
      if (p.life <= 0) G.particles.splice(i, 1);
    }
  }

  /* ============ GAME LOOP ============ */

  function loop() {
    Engine.update(G.engine, 1000 / 60);
    updateParticles();
    checkBird();
    refreshUI();
    draw();
    requestAnimationFrame(loop);
  }

  function checkBird() {
    if (G.phase !== 'flying' || !G.currentBird) return;
    const v = G.currentBird.velocity;
    const spd = Math.sqrt(v.x * v.x + v.y * v.y);
    const p = G.currentBird.position;

    if (p.x > CANVAS_W + 120 || p.x < -120 || p.y > CANVAS_H + 120) {
      birdDone();
      return;
    }

    if (spd < SETTLE_SPEED) {
      G.settleTimer++;
      if (G.settleTimer > SETTLE_FRAMES) birdDone();
    } else {
      G.settleTimer = 0;
    }
  }

  function birdDone() {
    if (G.currentBird) {
      World.remove(G.engine.world, G.currentBird);
      G.currentBird = null;
    }

    if (G.pigs.length === 0) {
      G.phase = 'won';
      const bonus = (G.totalBirds - G.birdUsed + 1) * 10000;
      G.score += bonus;
      refreshUI();
      const stars = bonus >= 30000 ? '⭐⭐⭐' : bonus >= 20000 ? '⭐⭐' : '⭐';
      const isLast = G.levelIdx >= LEVELS.length - 1;
      showOverlay('🎉 關卡通過！', `得分: ${G.score}`, stars, isLast ? '🤖 全部完成！' : '下一關 ▶');
      return;
    }

    if (G.birdUsed < G.totalBirds) {
      spawnBird();
    } else {
      G.phase = 'lost';
      showOverlay('💥 挑戰失敗', `得分: ${G.score}`, '', '🔄 重試');
    }
  }

  /* ============ DRAW ============ */

  function draw() {
    const ctx = G.ctx;

    /* ---- background ---- */
    const grad = ctx.createLinearGradient(0, 0, 0, CANVAS_H);
    grad.addColorStop(0, '#3B7DD8');
    grad.addColorStop(0.5, '#6EB1F0');
    grad.addColorStop(0.85, '#A8D8EA');
    grad.addColorStop(1, '#D4EAF7');
    ctx.fillStyle = grad;
    ctx.fillRect(0, 0, CANVAS_W, CANVAS_H);

    drawClouds(ctx);
    drawHills(ctx);

    /* ---- ground ---- */
    const gGrad = ctx.createLinearGradient(0, GROUND_Y, 0, CANVAS_H);
    gGrad.addColorStop(0, '#5DAE3B');
    gGrad.addColorStop(0.1, '#4A9A2D');
    gGrad.addColorStop(1, '#3D7A28');
    ctx.fillStyle = gGrad;
    ctx.fillRect(0, GROUND_Y, CANVAS_W, CANVAS_H - GROUND_Y);

    ctx.fillStyle = '#6BBF48';
    ctx.fillRect(0, GROUND_Y - 4, CANVAS_W, 8);
    ctx.fillStyle = '#3D7A28';
    ctx.fillRect(0, GROUND_Y, CANVAS_W, 3);

    /* ---- game objects ---- */
    drawSlingshot(ctx);

    for (const b of G.blocks) drawBlock(ctx, b);
    for (const p of G.pigs) drawPig(ctx, p);

    if (G.currentBird) {
      const aiming = G.phase === 'aiming';
      drawBird(ctx, G.currentBird, aiming);
      if (aiming && G.dragEnd) drawAimLine(ctx);
    }

    drawParticles(ctx);
  }

  /* ---- clouds ---- */

  function drawClouds(ctx) {
    ctx.fillStyle = 'rgba(255,255,255,0.7)';
    const list = [
      { x: 80, y: 70, w: 140, h: 45 },
      { x: 380, y: 50, w: 180, h: 55 },
      { x: 650, y: 85, w: 120, h: 38 },
      { x: 950, y: 45, w: 160, h: 50 },
      { x: 1200, y: 80, w: 130, h: 42 },
    ];
    for (const c of list) {
      ctx.beginPath();
      ctx.ellipse(c.x, c.y, c.w / 2, c.h / 2, 0, 0, Math.PI * 2);
      ctx.fill();
      ctx.beginPath();
      ctx.ellipse(c.x - c.w * 0.3, c.y + 4, c.w * 0.32, c.h * 0.45, 0, 0, Math.PI * 2);
      ctx.fill();
      ctx.beginPath();
      ctx.ellipse(c.x + c.w * 0.28, c.y + 2, c.w * 0.28, c.h * 0.42, 0, 0, Math.PI * 2);
      ctx.fill();
    }
  }

  /* ---- hills ---- */

  function drawHills(ctx) {
    ctx.fillStyle = '#7DBF5A';
    ctx.beginPath();
    ctx.moveTo(0, GROUND_Y);
    ctx.quadraticCurveTo(150, GROUND_Y - 50, 300, GROUND_Y - 15);
    ctx.quadraticCurveTo(450, GROUND_Y + 5, 600, GROUND_Y - 25);
    ctx.quadraticCurveTo(750, GROUND_Y - 55, 900, GROUND_Y - 10);
    ctx.quadraticCurveTo(1050, GROUND_Y + 5, 1200, GROUND_Y - 20);
    ctx.quadraticCurveTo(1350, GROUND_Y - 40, CANVAS_W, GROUND_Y - 5);
    ctx.lineTo(CANVAS_W, GROUND_Y);
    ctx.lineTo(0, GROUND_Y);
    ctx.closePath();
    ctx.fill();

    ctx.fillStyle = '#8FCB6A';
    ctx.beginPath();
    ctx.moveTo(0, GROUND_Y);
    ctx.quadraticCurveTo(200, GROUND_Y - 30, 400, GROUND_Y - 8);
    ctx.quadraticCurveTo(600, GROUND_Y + 2, 800, GROUND_Y - 35);
    ctx.quadraticCurveTo(1000, GROUND_Y - 60, 1200, GROUND_Y - 15);
    ctx.quadraticCurveTo(1350, GROUND_Y - 5, CANVAS_W, GROUND_Y - 20);
    ctx.lineTo(CANVAS_W, GROUND_Y);
    ctx.lineTo(0, GROUND_Y);
    ctx.closePath();
    ctx.fill();
  }

  /* ---- slingshot ---- */

  function drawSlingshot(ctx) {
    const sx = SLING_X, sy = SLING_Y;
    const lx = SLING_FORK_L.x, ly = SLING_FORK_L.y;
    const rx = SLING_FORK_R.x, ry = SLING_FORK_R.y;

    ctx.lineCap = 'round';

    /* back fork (drawn before body so body covers it) */
    ctx.strokeStyle = '#4A2C0A';
    ctx.lineWidth = 7;
    ctx.beginPath();
    ctx.moveTo(sx, sy + 70);
    ctx.lineTo(lx, ly);
    ctx.stroke();

    /* main post */
    ctx.strokeStyle = '#6B3F1A';
    ctx.lineWidth = 8;
    ctx.beginPath();
    ctx.moveTo(sx, sy + 85);
    ctx.lineTo(sx, sy - 5);
    ctx.stroke();

    /* left fork final */
    ctx.strokeStyle = '#5D3414';
    ctx.lineWidth = 7;
    ctx.beginPath();
    ctx.moveTo(sx, sy - 12);
    ctx.lineTo(lx, ly);
    ctx.stroke();

    /* right fork */
    ctx.strokeStyle = '#5D3414';
    ctx.lineWidth = 7;
    ctx.beginPath();
    ctx.moveTo(sx, sy - 12);
    ctx.lineTo(rx, ry);
    ctx.stroke();

    /* fork nubs */
    ctx.fillStyle = '#4A2C0A';
    ctx.beginPath();
    ctx.arc(lx, ly, 4.5, 0, Math.PI * 2);
    ctx.fill();
    ctx.beginPath();
    ctx.arc(rx, ry, 4.5, 0, Math.PI * 2);
    ctx.fill();

    /* rubber bands */
    if (G.phase === 'aiming' && G.currentBird && G.dragEnd) {
      const bx = G.currentBird.position.x;
      const by = G.currentBird.position.y;

      /* stretch lines */
      ctx.strokeStyle = '#2C1A04';
      ctx.lineWidth = 4;
      ctx.lineCap = 'round';
      ctx.beginPath();
      ctx.moveTo(lx, ly);
      ctx.lineTo(bx, by);
      ctx.stroke();
      ctx.beginPath();
      ctx.moveTo(rx, ry);
      ctx.lineTo(bx, by);
      ctx.stroke();

      /* bands thickness */
      ctx.strokeStyle = '#3E2408';
      ctx.lineWidth = 2.5;
      ctx.beginPath();
      ctx.moveTo(lx, ly);
      ctx.lineTo(bx, by);
      ctx.stroke();
      ctx.beginPath();
      ctx.moveTo(rx, ry);
      ctx.lineTo(bx, by);
      ctx.stroke();
    }
  }

  /* ---- aim line ---- */

  function drawAimLine(ctx) {
    const dx = SLING_X - G.dragEnd.x;
    const dy = SLING_Y - G.dragEnd.y;
    const d = Math.sqrt(dx * dx + dy * dy);
    if (d < 15) return;
    const capped = Math.min(d, MAX_DRAG);
    const r = capped / d;
    const vx = dx * r * LAUNCH_FORCE * 60;
    const vy = dy * r * LAUNCH_FORCE * 60;
    const g = G.engine.gravity.y;

    ctx.save();
    ctx.strokeStyle = 'rgba(255,255,255,0.35)';
    ctx.lineWidth = 2;
    ctx.setLineDash([6, 8]);
    ctx.beginPath();
    const steps = 30;
    for (let i = 0; i < steps; i++) {
      const t = i * 0.6;
      const px = SLING_X + vx * t;
      const py = SLING_Y + vy * t + 0.5 * g * 60 * t * t;
      if (i === 0) ctx.moveTo(px, py);
      else ctx.lineTo(px, py);
      if (py > GROUND_Y) break;
    }
    ctx.stroke();
    ctx.restore();
  }

  /* ---- bird ---- */

  function drawBird(ctx, body, aiming) {
    const x = body.position.x;
    const y = body.position.y;
    const r = BIRD_RADIUS;

    /* shadow */
    ctx.fillStyle = 'rgba(0,0,0,0.18)';
    ctx.beginPath();
    ctx.ellipse(x + 3, y + 5, r * 0.9, r * 0.35, 0.2, 0, Math.PI * 2);
    ctx.fill();

    /* body */
    const grad = ctx.createRadialGradient(x - 4, y - 4, 2, x, y, r);
    grad.addColorStop(0, '#FF6B6B');
    grad.addColorStop(0.7, '#E53935');
    grad.addColorStop(1, '#B71C1C');
    ctx.fillStyle = grad;
    ctx.beginPath();
    ctx.arc(x, y, r, 0, Math.PI * 2);
    ctx.fill();

    ctx.strokeStyle = '#8E0000';
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.arc(x, y, r, 0, Math.PI * 2);
    ctx.stroke();

    /* belly */
    ctx.fillStyle = '#FFCDD2';
    ctx.beginPath();
    ctx.ellipse(x - 1, y + 4, r * 0.45, r * 0.55, 0, 0, Math.PI * 2);
    ctx.fill();

    /* eyes */
    ctx.fillStyle = '#fff';
    ctx.beginPath();
    ctx.arc(x - 5, y - 4, 4.5, 0, Math.PI * 2);
    ctx.fill();
    ctx.beginPath();
    ctx.arc(x + 5, y - 4, 4.5, 0, Math.PI * 2);
    ctx.fill();

    ctx.fillStyle = '#1A1A1A';
    ctx.beginPath();
    ctx.arc(x - 4, y - 3, 2.5, 0, Math.PI * 2);
    ctx.fill();
    ctx.beginPath();
    ctx.arc(x + 6, y - 3, 2.5, 0, Math.PI * 2);
    ctx.fill();

    /* eyebrows */
    ctx.strokeStyle = '#3E2723';
    ctx.lineWidth = 2.5;
    ctx.beginPath();
    ctx.moveTo(x - 11, y - 13);
    ctx.lineTo(x - 3, y - 10);
    ctx.stroke();
    ctx.beginPath();
    ctx.moveTo(x + 11, y - 13);
    ctx.lineTo(x + 3, y - 10);
    ctx.stroke();

    /* beak */
    ctx.fillStyle = '#FF8F00';
    ctx.beginPath();
    ctx.moveTo(x + 8, y + 1);
    ctx.lineTo(x + 19, y + 4);
    ctx.lineTo(x + 8, y + 7);
    ctx.closePath();
    ctx.fill();

    /* tail */
    ctx.fillStyle = '#8E0000';
    ctx.beginPath();
    ctx.moveTo(x - r + 1, y - 2);
    ctx.lineTo(x - r - 10, y - 11);
    ctx.lineTo(x - r - 6, y - 2);
    ctx.lineTo(x - r - 11, y + 4);
    ctx.lineTo(x - r - 3, y + 3);
    ctx.closePath();
    ctx.fill();

    /* aiming glow */
    if (aiming) {
      ctx.strokeStyle = 'rgba(255,200,50,0.3)';
      ctx.lineWidth = 3;
      ctx.setLineDash([4, 6]);
      ctx.beginPath();
      ctx.arc(x, y, r + 6, 0, Math.PI * 2);
      ctx.stroke();
      ctx.setLineDash([]);
    }
  }

  /* ---- pig ---- */

  function drawPig(ctx, body) {
    const x = body.position.x;
    const y = body.position.y;
    const r = body.data.r;
    const hpPct = Math.max(0, body.data.hp / body.data.maxHp);
    const dmg = 1 - hpPct;

    ctx.save();

    /* shadow */
    ctx.fillStyle = 'rgba(0,0,0,0.15)';
    ctx.beginPath();
    ctx.ellipse(x + 2, y + r * 0.5 + 3, r * 0.75, r * 0.25, 0, 0, Math.PI * 2);
    ctx.fill();

    /* body */
    const g = Math.round(170 - dmg * 60);
    const rCol = Math.round(60 + dmg * 60);
    ctx.fillStyle = `rgb(${rCol}, ${g}, 50)`;
    ctx.beginPath();
    ctx.arc(x, y, r, 0, Math.PI * 2);
    ctx.fill();

    ctx.strokeStyle = `rgba(30,80,20,${0.5 + dmg * 0.5})`;
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.arc(x, y, r, 0, Math.PI * 2);
    ctx.stroke();

    /* snout */
    ctx.fillStyle = `rgb(${Math.round(80 + dmg * 40)}, ${Math.round(160 - dmg * 40)}, 60)`;
    ctx.beginPath();
    ctx.ellipse(x, y + 4, r * 0.38, r * 0.32, 0, 0, Math.PI * 2);
    ctx.fill();
    ctx.strokeStyle = '#2E5A20';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.ellipse(x, y + 4, r * 0.38, r * 0.32, 0, 0, Math.PI * 2);
    ctx.stroke();

    /* nostrils */
    ctx.fillStyle = '#2E5A20';
    ctx.beginPath();
    ctx.ellipse(x - 3, y + 4, 2, 1.5, 0, 0, Math.PI * 2);
    ctx.fill();
    ctx.beginPath();
    ctx.ellipse(x + 3, y + 4, 2, 1.5, 0, 0, Math.PI * 2);
    ctx.fill();

    /* eyes */
    const eyeY = y - 5 - dmg * 2;
    ctx.fillStyle = '#fff';
    ctx.beginPath();
    ctx.arc(x - 5, eyeY, 4.5, 0, Math.PI * 2);
    ctx.fill();
    ctx.beginPath();
    ctx.arc(x + 5, eyeY, 4.5, 0, Math.PI * 2);
    ctx.fill();

    ctx.fillStyle = '#1A1A1A';
    const pupilOff = dmg > 0.5 ? 1 : 0;
    ctx.beginPath();
    ctx.arc(x - 4 + pupilOff, eyeY + 1, 2.5, 0, Math.PI * 2);
    ctx.fill();
    ctx.beginPath();
    ctx.arc(x + 6 - pupilOff, eyeY + 1, 2.5, 0, Math.PI * 2);
    ctx.fill();

    /* ears */
    ctx.fillStyle = `rgb(${rCol}, ${g}, 50)`;
    ctx.beginPath();
    ctx.ellipse(x - r * 0.7, y - r * 0.7, r * 0.35, r * 0.45, -0.3, 0, Math.PI * 2);
    ctx.fill();
    ctx.beginPath();
    ctx.ellipse(x + r * 0.7, y - r * 0.7, r * 0.35, r * 0.45, 0.3, 0, Math.PI * 2);
    ctx.fill();

    /* angry expression when damaged */
    if (dmg > 0.3) {
      ctx.strokeStyle = '#1A1A1A';
      ctx.lineWidth = 1.5;
      ctx.beginPath();
      ctx.moveTo(x - 9 + dmg * 2, eyeY + 6);
      ctx.lineTo(x - 3 + dmg * 2, eyeY + 4);
      ctx.stroke();
      ctx.beginPath();
      ctx.moveTo(x + 9 - dmg * 2, eyeY + 6);
      ctx.lineTo(x + 3 - dmg * 2, eyeY + 4);
      ctx.stroke();
    }

    ctx.restore();
  }

  /* ---- block ---- */

  function drawBlock(ctx, body) {
    const verts = body.vertices;
    const hpPct = Math.max(0, body.data.hp / body.data.maxHp);
    const v = verts;

    ctx.save();

    /* shadow */
    ctx.fillStyle = 'rgba(0,0,0,0.1)';
    ctx.beginPath();
    ctx.moveTo(v[0].x + 3, v[0].y + 4);
    for (let i = 1; i < v.length; i++) ctx.lineTo(v[i].x + 3, v[i].y + 4);
    ctx.closePath();
    ctx.fill();

    /* body */
    const color = body._color;
    ctx.fillStyle = color;
    ctx.beginPath();
    ctx.moveTo(v[0].x, v[0].y);
    for (let i = 1; i < v.length; i++) ctx.lineTo(v[i].x, v[i].y);
    ctx.closePath();
    ctx.fill();

    /* outline */
    ctx.strokeStyle = `rgba(0,0,0,${0.25 + (1 - hpPct) * 0.3})`;
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.moveTo(v[0].x, v[0].y);
    for (let i = 1; i < v.length; i++) ctx.lineTo(v[i].x, v[i].y);
    ctx.closePath();
    ctx.stroke();

    /* damage */
    if (hpPct < 0.75) {
      ctx.strokeStyle = `rgba(0,0,0,${0.2 * (1 - hpPct)})`;
      ctx.lineWidth = 1.2;
      const cx = (v[0].x + v[2].x) / 2;
      const cy = (v[0].y + v[2].y) / 2;
      ctx.beginPath();
      ctx.moveTo(cx - 6, cy - 6);
      ctx.lineTo(cx + 3, cy + 1);
      ctx.lineTo(cx - 2, cy + 7);
      ctx.stroke();
      ctx.beginPath();
      ctx.moveTo(cx - 1, cy - 4);
      ctx.lineTo(cx + 6, cy + 3);
      ctx.stroke();
    }

    ctx.restore();
  }

  /* ---- particles ---- */

  function drawParticles(ctx) {
    for (const p of G.particles) {
      ctx.globalAlpha = Math.max(0, p.life);
      ctx.fillStyle = p.color;
      ctx.fillRect(p.x - p.size / 2, p.y - p.size / 2, p.size, p.size);
    }
    ctx.globalAlpha = 1;
  }

  /* ============ UI ============ */

  function refreshUI() {
    document.getElementById('score-info').textContent = `分數: ${G.score}`;
    const rem = G.totalBirds - G.birdUsed + (G.currentBird && G.phase !== 'flying' ? 1 : 0);
    document.getElementById('birds-info').textContent = `小鳥: ${Math.max(0, rem)}`;
  }

  function showOverlay(title, score, stars, btn) {
    const ov = document.getElementById('ui-overlay');
    ov.classList.remove('hidden');
    document.getElementById('overlay-title').textContent = title;
    document.getElementById('overlay-score').textContent = score;
    document.getElementById('overlay-stars').textContent = stars || '';
    const b = document.getElementById('overlay-btn');
    b.textContent = btn;
    b.onclick = () => {
      if (btn.includes('下一關')) loadLevel(G.levelIdx + 1);
      else if (btn.includes('全部完成')) loadLevel(0);
      else loadLevel(G.levelIdx);
    };
  }

  function hideOverlay() {
    document.getElementById('ui-overlay').classList.add('hidden');
  }

  /* ============ START ============ */

  window.addEventListener('DOMContentLoaded', init);
})();
