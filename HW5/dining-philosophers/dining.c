/*
 * HW5 - 哲學家用餐問題
 *
 * 經典的同步問題：五位哲學家坐在圓桌前，
 * 每個人需要兩根筷子才能吃飯，但只有五根筷子。
 * 本實作使用「固定取得順序」預防死結。
 *
 * 編譯：gcc -pthread -o dining dining.c && ./dining
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#define NUM_PHILOSOPHERS  5     // 哲學家數量
#define MAX_EAT_TIMES     3     // 每位哲學家吃飯次數

/* 筷子（互斥鎖） */
pthread_mutex_t chopsticks[NUM_PHILOSOPHERS];

/* 哲學家狀態 */
typedef enum {
    THINKING,
    HUNGRY,
    EATING
} State;

State state[NUM_PHILOSOPHERS];
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

/* 取得筷子編號（左手、右手） */
#define LEFT(i)  ((i) % NUM_PHILOSOPHERS)
#define RIGHT(i) (((i) + 1) % NUM_PHILOSOPHERS)

/* 安全輸出（避免多執行緒輸出交錯） */
void safe_printf(int id, const char *fmt, ...)
{
    va_list args;
    pthread_mutex_lock(&print_mutex);
    printf("哲學家 %d: ", id);
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    pthread_mutex_unlock(&print_mutex);
}

/* 思考 */
void think(int id)
{
    safe_printf(id, "🤔 思考中...");
    usleep(rand() % 500000);  // 隨機思考時間
}

/* 吃飯 */
void eat(int id)
{
    safe_printf(id, "🍝 吃飯中...");
    usleep(rand() % 300000);  // 隨機吃飯時間
}

/*
 * 取得筷子（死結預防版本）
 *
 * 策略：所有哲學家以「固定順序」取得筷子
 * 先拿編號小的筷子，再拿編號大的筷子
 * 如此破壞「循環等待」條件，預防死結
 */
void pickup_chopsticks(int id)
{
    int left  = LEFT(id);   // 左手筷子編號
    int right = RIGHT(id);  // 右手筷子編號

    state[id] = HUNGRY;
    safe_printf(id, "🍽️  餓了，準備拿筷子...");

    /*
     * 死結預防：固定順序
     *
     * 原始版本（可能死結）：
     *   pthread_mutex_lock(&chopsticks[left]);
     *   pthread_mutex_lock(&chopsticks[right]);
     *
     * 修正版本：先鎖編號小的筷子
     * 破壞循環等待條件！
     */
    int first  = (left  < right) ? left  : right;
    int second = (left  < right) ? right : left;

    pthread_mutex_lock(&chopsticks[first]);
    safe_printf(id, "🥢 拿起筷子 %d", first);
    usleep(10000);  // 刻意延遲凸顯死結預防效果

    pthread_mutex_lock(&chopsticks[second]);
    safe_printf(id, "🥢 拿起筷子 %d (現在有兩根了!)", second);

    state[id] = EATING;
}

/* 放下筷子 */
void putdown_chopsticks(int id)
{
    int left  = LEFT(id);
    int right = RIGHT(id);

    state[id] = THINKING;

    pthread_mutex_unlock(&chopsticks[left]);
    pthread_mutex_unlock(&chopsticks[right]);
    safe_printf(id, "放下筷子");
}

/* 哲學家執行緒 */
void *philosopher(void *arg)
{
    int id = *(int *)arg;
    free(arg);

    for (int i = 0; i < MAX_EAT_TIMES; i++) {
        think(id);                  // 思考
        pickup_chopsticks(id);      // 拿筷子（含死結預防）
        eat(id);                    // 吃飯
        putdown_chopsticks(id);     // 放下筷子
    }

    safe_printf(id, "✓ 吃飽了");
    return NULL;
}

int main()
{
    pthread_t philosophers[NUM_PHILOSOPHERS];

    /* 初始化筷子（mutex） */
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pthread_mutex_init(&chopsticks[i], NULL);
        state[i] = THINKING;
    }

    printf("哲學家用餐問題模擬\n");
    printf("========================================\n");
    printf("哲學家數量: %d\n", NUM_PHILOSOPHERS);
    printf("每位吃飯次數: %d\n", MAX_EAT_TIMES);
    printf("死結預防策略: 固定鎖定順序\n");
    printf("========================================\n\n");

    srand(time(NULL));

    /* 建立哲學家執行緒 */
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        int *id = malloc(sizeof(int));
        *id = i;
        if (pthread_create(&philosophers[i], NULL, philosopher, id) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    /* 等待所有哲學家吃完 */
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pthread_join(philosophers[i], NULL);
    }

    printf("\n✓ 所有哲學家都吃完！沒有發生死結。\n");

    /* 清理 */
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pthread_mutex_destroy(&chopsticks[i]);
    }
    pthread_mutex_destroy(&print_mutex);

    return 0;
}
