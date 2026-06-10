/*
 * HW5 - 銀行存提款模擬程式
 *
 * 模擬同一個帳戶進行 100000 次存款和 100000 次提款，
 * 使用 mutex 保護帳戶餘額，確保最終金額正確。
 *
 * 編譯：gcc -pthread -o bank bank.c && ./bank
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define INITIAL_BALANCE   1000     // 初始存款
#define DEPOSIT_AMOUNT    1        // 每次存款金額
#define WITHDRAW_AMOUNT   1        // 每次提款金額
#define TRANSACTIONS      100000   // 每種交易次數

/* 帳戶結構 */
typedef struct {
    int balance;                // 目前餘額
    pthread_mutex_t mutex;      // 保護餘額的互斥鎖
    int deposit_count;          // 存款成功次數
    int withdraw_count;         // 提款成功次數
    int insufficient_count;     // 餘額不足次數
} Account;

/* 執行緒參數 */
typedef struct {
    Account *account;
} ThreadArg;

/* 存款執行緒 */
void *deposit_thread(void *arg)
{
    ThreadArg *ta = (ThreadArg *)arg;
    Account *acct = ta->account;

    for (int i = 0; i < TRANSACTIONS; i++) {
        pthread_mutex_lock(&acct->mutex);
        acct->balance += DEPOSIT_AMOUNT;
        acct->deposit_count++;
        pthread_mutex_unlock(&acct->mutex);
    }
    return NULL;
}

/* 提款執行緒 */
void *withdraw_thread(void *arg)
{
    ThreadArg *ta = (ThreadArg *)arg;
    Account *acct = ta->account;

    for (int i = 0; i < TRANSACTIONS; i++) {
        pthread_mutex_lock(&acct->mutex);
        if (acct->balance >= WITHDRAW_AMOUNT) {
            acct->balance -= WITHDRAW_AMOUNT;
            acct->withdraw_count++;
        } else {
            acct->insufficient_count++;
        }
        pthread_mutex_unlock(&acct->mutex);
    }
    return NULL;
}

int main()
{
    Account account = {
        .balance           = INITIAL_BALANCE,
        .mutex             = PTHREAD_MUTEX_INITIALIZER,
        .deposit_count     = 0,
        .withdraw_count    = 0,
        .insufficient_count = 0
    };

    pthread_t deposit_tid, withdraw_tid;
    ThreadArg arg = { .account = &account };

    printf("銀行存提款模擬\n");
    printf("========================================\n");
    printf("初始餘額:          %d 元\n", INITIAL_BALANCE);
    printf("存款次數:          %d 次 (每次 %d 元)\n", TRANSACTIONS, DEPOSIT_AMOUNT);
    printf("提款次數:          %d 次 (每次 %d 元)\n", TRANSACTIONS, WITHDRAW_AMOUNT);
    printf("========================================\n\n");

    /* 建立存款與提款執行緒 */
    if (pthread_create(&deposit_tid, NULL, deposit_thread, &arg) != 0) {
        perror("pthread_create (deposit)");
        return 1;
    }
    if (pthread_create(&withdraw_tid, NULL, withdraw_thread, &arg) != 0) {
        perror("pthread_create (withdraw)");
        return 1;
    }

    /* 等待執行緒結束 */
    pthread_join(deposit_tid, NULL);
    pthread_join(withdraw_tid, NULL);

    /* 計算預期結果 */
    int expected = INITIAL_BALANCE
                 + TRANSACTIONS * DEPOSIT_AMOUNT
                 - account.withdraw_count * WITHDRAW_AMOUNT;

    printf("========================================\n");
    printf("執行結果\n");
    printf("========================================\n");
    printf("實際餘額:          %d 元\n", account.balance);
    printf("預期餘額:          %d 元\n", expected);
    printf("存款成功次數:      %d 次\n", account.deposit_count);
    printf("提款成功次數:      %d 次\n", account.withdraw_count);
    printf("餘額不足次數:      %d 次\n", account.insufficient_count);

    if (account.balance == expected) {
        printf("\n✓ 餘額正確！Mutex 有效防止了競爭條件。\n");
    } else {
        printf("\n✗ 餘額錯誤！存在競爭條件。\n");
    }

    pthread_mutex_destroy(&account.mutex);
    return 0;
}
