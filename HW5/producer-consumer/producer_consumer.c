/*
 * HW5 - 生產者消費者問題
 *
 * 經典的同步問題：生產者生產資料放入緩衝區，消費者從緩衝區取出資料處理。
 * 使用 mutex + 條件變數（或號誌）實作。
 *
 * 編譯：gcc -pthread -o producer_consumer producer_consumer.c && ./producer_consumer
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#define BUFFER_SIZE  8       // 緩衝區大小
#define ITEM_COUNT   20      // 生產/消費總數量

/* 環形緩衝區 */
typedef struct {
    int buffer[BUFFER_SIZE]; // 資料緩衝區
    int in;                  // 下一個生產位置
    int out;                 // 下一個消費位置
    int count;               // 目前項目數量
    pthread_mutex_t mutex;   // 互斥鎖
    pthread_cond_t not_full; // 緩衝區未滿條件變數
    pthread_cond_t not_empty;// 緩衝區非空條件變數
} RingBuffer;

/* 初始化緩衝區 */
void rb_init(RingBuffer *rb)
{
    rb->in    = 0;
    rb->out   = 0;
    rb->count = 0;
    pthread_mutex_init(&rb->mutex, NULL);
    pthread_cond_init(&rb->not_full, NULL);
    pthread_cond_init(&rb->not_empty, NULL);
}

/* 生產一個項目 */
void rb_put(RingBuffer *rb, int value)
{
    pthread_mutex_lock(&rb->mutex);

    /* 緩衝區滿了則等待 */
    while (rb->count >= BUFFER_SIZE) {
        pthread_cond_wait(&rb->not_full, &rb->mutex);
    }

    /* 放入資料 */
    rb->buffer[rb->in] = value;
    rb->in  = (rb->in + 1) % BUFFER_SIZE;
    rb->count++;

    printf("生產者 → [%2d]  (緩衝區: %d/%d)\n", value, rb->count, BUFFER_SIZE);

    /* 通知消費者緩衝區非空 */
    pthread_cond_signal(&rb->not_empty);
    pthread_mutex_unlock(&rb->mutex);
}

/* 消費一個項目 */
int rb_get(RingBuffer *rb)
{
    pthread_mutex_lock(&rb->mutex);

    /* 緩衝區空了則等待 */
    while (rb->count <= 0) {
        pthread_cond_wait(&rb->not_empty, &rb->mutex);
    }

    /* 取出資料 */
    int value = rb->buffer[rb->out];
    rb->out  = (rb->out + 1) % BUFFER_SIZE;
    rb->count--;

    printf("消費者 ← [%2d]  (緩衝區: %d/%d)\n", value, rb->count, BUFFER_SIZE);

    /* 通知生產者緩衝區未滿 */
    pthread_cond_signal(&rb->not_full);
    pthread_mutex_unlock(&rb->mutex);

    return value;
}

/* 緩衝區資料 */
typedef struct {
    RingBuffer *rb;
    int id;
} ThreadArg;

/* 生產者執行緒 */
void *producer(void *arg)
{
    ThreadArg *ta = (ThreadArg *)arg;

    for (int i = 0; i < ITEM_COUNT; i++) {
        int value = ta->id * 100 + i;  // 生產項目編號
        rb_put(ta->rb, value);
        usleep(rand() % 200000);       // 隨機延遲
    }
    return NULL;
}

/* 消費者執行緒 */
void *consumer(void *arg)
{
    ThreadArg *ta = (ThreadArg *)arg;

    for (int i = 0; i < ITEM_COUNT; i++) {
        int value = rb_get(ta->rb);
        usleep(rand() % 300000);       // 隨機延遲
    }
    return NULL;
}

int main()
{
    RingBuffer rb;
    rb_init(&rb);

    pthread_t prod_tid, cons_tid;
    ThreadArg prod_arg = { .rb = &rb, .id = 1 };
    ThreadArg cons_arg = { .rb = &rb, .id = 1 };

    printf("生產者消費者問題模擬\n");
    printf("緩衝區大小: %d\n", BUFFER_SIZE);
    printf("生產/消費數量: %d\n\n", ITEM_COUNT);

    srand(time(NULL));

    /* 建立生產者與消費者執行緒 */
    if (pthread_create(&prod_tid, NULL, producer, &prod_arg) != 0) {
        perror("pthread_create (producer)");
        return 1;
    }
    if (pthread_create(&cons_tid, NULL, consumer, &cons_arg) != 0) {
        perror("pthread_create (consumer)");
        return 1;
    }

    /* 等待結束 */
    pthread_join(prod_tid, NULL);
    pthread_join(cons_tid, NULL);

    printf("\n✓ 生產者消費者模擬完成\n");

    pthread_mutex_destroy(&rb.mutex);
    pthread_cond_destroy(&rb.not_full);
    pthread_cond_destroy(&rb.not_empty);

    return 0;
}
