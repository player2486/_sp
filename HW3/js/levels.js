const LEVELS = [
  {
    name: '初試啼聲',
    birds: 3,
    pigs: [
      { x: 800, y: 536, size: 'small' },
      { x: 800, y: 471, size: 'small' },
    ],
    blocks: [
      { x: 760, y: 525, w: 15, h: 50, type: 'wood' },
      { x: 840, y: 525, w: 15, h: 50, type: 'wood' },
      { x: 800, y: 493, w: 100, h: 15, type: 'wood' },
    ],
  },
  {
    name: '雙重堡壘',
    birds: 3,
    pigs: [
      { x: 750, y: 471, size: 'small' },
      { x: 870, y: 471, size: 'small' },
      { x: 870, y: 536, size: 'small' },
    ],
    blocks: [
      { x: 720, y: 525, w: 15, h: 50, type: 'wood' },
      { x: 780, y: 525, w: 15, h: 50, type: 'wood' },
      { x: 750, y: 493, w: 70, h: 15, type: 'wood' },
      { x: 840, y: 525, w: 15, h: 50, type: 'wood' },
      { x: 900, y: 525, w: 15, h: 50, type: 'wood' },
      { x: 870, y: 493, w: 70, h: 15, type: 'wood' },
    ],
  },
  {
    name: '石頭要塞',
    birds: 4,
    pigs: [
      { x: 810, y: 536, size: 'medium' },
      { x: 770, y: 467, size: 'medium' },
      { x: 850, y: 471, size: 'small' },
    ],
    blocks: [
      { x: 735, y: 525, w: 20, h: 50, type: 'stone' },
      { x: 790, y: 525, w: 15, h: 50, type: 'wood' },
      { x: 830, y: 525, w: 15, h: 50, type: 'wood' },
      { x: 885, y: 525, w: 20, h: 50, type: 'stone' },
      { x: 810, y: 493, w: 170, h: 15, type: 'wood' },
      { x: 810, y: 460, w: 120, h: 12, type: 'glass' },
    ],
  },
];
