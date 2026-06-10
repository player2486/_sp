/*
 * 114b期中 - Mini Shell (微殼)
 *
 * 功能：
 *   - 外部命令執行 (fork + execvp)
 *   - 管線 | (pipe + dup2)
 *   - 重新導向 > < >> 2>
 *   - 背景執行 &
 *   - 內建指令: cd, exit, jobs, kill
 *   - 信號處理: SIGINT, SIGCHLD
 *
 * 編譯: gcc -Wall -Wextra -o msh msh.c
 * 執行: ./msh
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>

#define MAX_LINE     1024    /* 命令列最大長度 */
#define MAX_ARGS      128    /* 每個命令最大參數數 */
#define MAX_JOBS       64    /* 最大背景行程數 */
#define MAX_CMD        32    /* 管線最大命令數 */

/* 重新導向類型 */
typedef enum { REDIR_NONE, REDIR_IN, REDIR_OUT, REDIR_APPEND, REDIR_ERR } RedirType;

/* 單一命令 */
typedef struct {
    char *argv[MAX_ARGS];    /* 參數陣列 */
    int argc;                /* 參數數量 */
    char *infile;            /* 輸入重新導向檔案 */
    char *outfile;           /* 輸出重新導向檔案 */
    char *errfile;           /* 錯誤重新導向檔案 */
    RedirType in_redir;      /* 輸入重新導向類型 */
    RedirType out_redir;     /* 輸出重新導向類型 */
    RedirType err_redir;     /* 錯誤重新導向類型 */
    int background;          /* 背景執行旗標 */
} Command;

/* 背景行程 */
typedef struct {
    pid_t pid;
    char cmd[MAX_LINE];
    int active;
} Job;

static Job jobs[MAX_JOBS];
static int job_count = 0;

/* 加入背景行程 */
static void job_add(pid_t pid, const char *cmd)
{
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!jobs[i].active) {
            jobs[i].pid = pid;
            strncpy(jobs[i].cmd, cmd, MAX_LINE - 1);
            jobs[i].active = 1;
            job_count++;
            printf("[%d] %d\n", i + 1, pid);
            return;
        }
    }
}

/* 移除背景行程 */
static void __attribute__((unused)) job_remove(pid_t pid)
{
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].active && jobs[i].pid == pid) {
            jobs[i].active = 0;
            job_count--;
            return;
        }
    }
}

/* 清理已完成背景行程 */
static void job_cleanup(void)
{
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].active) {
            int status;
            pid_t result = waitpid(jobs[i].pid, &status, WNOHANG);
            if (result > 0) {
                printf("\n[%d] ", i + 1);
                if (WIFEXITED(status))
                    printf("Done    %s\n", jobs[i].cmd);
                else if (WIFSIGNALED(status))
                    printf("Terminated %s\n", jobs[i].cmd);
                jobs[i].active = 0;
                job_count--;
                fflush(stdout);
            }
        }
    }
}

/* SIGCHLD 處理：清理子行程 */
static void sigchld_handler(int sig)
{
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

/* SIGINT 處理：忽略（不終止 Shell）*/
static void sigint_handler(int sig)
{
    (void)sig;
    write(STDOUT_FILENO, "\nmsh> ", 6);
    fflush(stdout);
}

/* 移除行尾換行 */
static void trim_newline(char *s)
{
    size_t len = strlen(s);
    if (len > 0 && s[len - 1] == '\n')
        s[len - 1] = '\0';
}

/* 分割命令列（支援引號）*/
static int parse_line(char *line, char **tokens, int max_tokens)
{
    int count = 0;
    char *p = line;

    while (*p && count < max_tokens) {
        /* 跳過空白 */
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;

        if (*p == '"' || *p == '\'') {
            /* 引號字串 */
            char quote = *p++;
            tokens[count] = p;
            while (*p && *p != quote) p++;
            if (*p) *p++ = '\0';
            count++;
        } else {
            /* 一般 token */
            tokens[count] = p;
            while (*p && !isspace((unsigned char)*p)) p++;
            if (*p) *p++ = '\0';
            count++;
        }
    }
    return count;
}

/* 解析重新導向，回傳實際命令參數數量 */
static int parse_redirect(char **tokens, int token_count, Command *cmd)
{
    int argc = 0;
    memset(cmd, 0, sizeof(Command));

    for (int i = 0; i < token_count; i++) {
        if (strcmp(tokens[i], ">") == 0 && i + 1 < token_count) {
            cmd->outfile = tokens[++i];
            cmd->out_redir = REDIR_OUT;
        } else if (strcmp(tokens[i], ">>") == 0 && i + 1 < token_count) {
            cmd->outfile = tokens[++i];
            cmd->out_redir = REDIR_APPEND;
        } else if (strcmp(tokens[i], "<") == 0 && i + 1 < token_count) {
            cmd->infile = tokens[++i];
            cmd->in_redir = REDIR_IN;
        } else if (strcmp(tokens[i], "2>") == 0 && i + 1 < token_count) {
            cmd->errfile = tokens[++i];
            cmd->err_redir = REDIR_ERR;
        } else if (strcmp(tokens[i], "&") == 0) {
            cmd->background = 1;
        } else {
            cmd->argv[argc++] = tokens[i];
        }
    }
    cmd->argv[argc] = NULL;
    cmd->argc = argc;
    return argc;
}

/* 執行單一命令（含重新導向）*/
static int execute_command(Command *cmd)
{
    /* 空命令 */
    if (cmd->argc == 0) return 0;

    /* 內建指令 */
    if (strcmp(cmd->argv[0], "exit") == 0)
        exit(0);

    if (strcmp(cmd->argv[0], "cd") == 0) {
        const char *path = cmd->argv[1] ? cmd->argv[1] : getenv("HOME");
        if (chdir(path) < 0)
            perror("cd");
        return 0;
    }

    if (strcmp(cmd->argv[0], "jobs") == 0) {
        for (int i = 0; i < MAX_JOBS; i++) {
            if (jobs[i].active)
                printf("[%d] %d    %s\n", i + 1, jobs[i].pid, jobs[i].cmd);
        }
        return 0;
    }

    if (strcmp(cmd->argv[0], "kill") == 0) {
        if (cmd->argc >= 2) {
            int sig = SIGTERM;
            int pid_start = 1;
            if (strcmp(cmd->argv[1], "-9") == 0) {
                sig = SIGKILL;
                pid_start = 2;
            }
            for (int i = pid_start; i < cmd->argc; i++) {
                pid_t pid = atoi(cmd->argv[i]);
                if (kill(pid, sig) < 0)
                    perror("kill");
            }
        }
        return 0;
    }

    /* 外部命令 */
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return -1; }

    if (pid == 0) {
        /* 子行程：設定重新導向 */

        /* 輸入重新導向 < */
        if (cmd->in_redir == REDIR_IN && cmd->infile) {
            int fd = open(cmd->infile, O_RDONLY);
            if (fd < 0) { perror(cmd->infile); exit(1); }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        /* 輸出重新導向 > 或 >> */
        if (cmd->out_redir != REDIR_NONE && cmd->outfile) {
            int flags = O_WRONLY | O_CREAT;
            if (cmd->out_redir == REDIR_APPEND)
                flags |= O_APPEND;
            else
                flags |= O_TRUNC;
            int fd = open(cmd->outfile, flags, 0644);
            if (fd < 0) { perror(cmd->outfile); exit(1); }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        /* 錯誤重新導向 2> */
        if (cmd->err_redir == REDIR_ERR && cmd->errfile) {
            int fd = open(cmd->errfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) { perror(cmd->errfile); exit(1); }
            dup2(fd, STDERR_FILENO);
            close(fd);
        }

        /* 還原 SIGINT/SIGCHLD 為預設行為 */
        signal(SIGINT, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);

        execvp(cmd->argv[0], cmd->argv);
        /* execvp 失敗 */
        fprintf(stderr, "msh: %s: command not found\n", cmd->argv[0]);
        exit(127);
    }

    /* 父行程 */
    if (cmd->background) {
        job_add(pid, cmd->argv[0]);
        return 0;
    }

    int status;
    waitpid(pid, &status, 0);
    return WEXITSTATUS(status);
}

/* 執行管線命令 */
static int execute_pipeline(Command *cmds, int cmd_count)
{
    if (cmd_count == 1) {
        return execute_command(&cmds[0]);
    }

    int prev_fd = -1;
    pid_t pids[MAX_CMD];
    int pid_count = 0;

    for (int i = 0; i < cmd_count; i++) {
        int pipefd[2];

        /* 非最後一個命令才建立 pipe */
        if (i < cmd_count - 1) {
            if (pipe(pipefd) < 0) { perror("pipe"); return -1; }
        }

        pid_t pid = fork();
        if (pid < 0) { perror("fork"); return -1; }

        if (pid == 0) {
            /* 子行程 */

            /* 從上一個 pipe 讀取（非第一個命令）*/
            if (prev_fd != -1) {
                dup2(prev_fd, STDIN_FILENO);
                close(prev_fd);
            }

            /* 寫入下一個 pipe（非最後一個命令）*/
            if (i < cmd_count - 1) {
                close(pipefd[0]);
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);
            }

            /* 處理此命令自己的重新導向 */
            Command *cmd = &cmds[i];
            if (cmd->in_redir == REDIR_IN && cmd->infile) {
                int fd = open(cmd->infile, O_RDONLY);
                if (fd < 0) { perror(cmd->infile); exit(1); }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            if (cmd->out_redir != REDIR_NONE && cmd->outfile) {
                int flags = O_WRONLY | O_CREAT;
                if (cmd->out_redir == REDIR_APPEND) flags |= O_APPEND;
                else flags |= O_TRUNC;
                int fd = open(cmd->outfile, flags, 0644);
                if (fd < 0) { perror(cmd->outfile); exit(1); }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
            if (cmd->err_redir == REDIR_ERR && cmd->errfile) {
                int fd = open(cmd->errfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) { perror(cmd->errfile); exit(1); }
                dup2(fd, STDERR_FILENO);
                close(fd);
            }

            signal(SIGINT, SIG_DFL);
            signal(SIGCHLD, SIG_DFL);

            execvp(cmd->argv[0], cmd->argv);
            fprintf(stderr, "msh: %s: command not found\n", cmd->argv[0]);
            exit(127);
        }

        pids[pid_count++] = pid;

        /* 關閉上一個 pipe 的讀取端 */
        if (prev_fd != -1) close(prev_fd);

        /* 保留此 pipe 的讀取端給下一個命令 */
        if (i < cmd_count - 1) {
            prev_fd = pipefd[0];
            close(pipefd[1]);
        }
    }

    /* 關閉最後一個讀取端 */
    if (prev_fd != -1) close(prev_fd);

    /* 等待所有子行程 */
    int last_status = 0;
    for (int i = 0; i < pid_count; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        if (i == pid_count - 1)
            last_status = WEXITSTATUS(status);
    }
    return last_status;
}

int main(int argc, char *argv[])
{
    /* 設定信號處理 */
    signal(SIGINT, sigint_handler);
    signal(SIGCHLD, sigchld_handler);
    signal(SIGTERM, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);

    /* 批次模式：-c "command" */
    if (argc >= 3 && strcmp(argv[1], "-c") == 0) {
        char line[MAX_LINE];
        strncpy(line, argv[2], MAX_LINE - 1);
        char *tokens[MAX_ARGS];
        int token_count = parse_line(line, tokens, MAX_ARGS);

        Command cmds[MAX_CMD];
        int cmd_count = 0;
        int start = 0;

        for (int i = 0; i <= token_count; i++) {
            if (i == token_count || strcmp(tokens[i], "|") == 0) {
                int n = parse_redirect(tokens + start, i - start, &cmds[cmd_count]);
                if (n > 0) cmd_count++;
                start = i + 1;
            }
        }

        if (cmd_count > 0)
            execute_pipeline(cmds, cmd_count);
        return 0;
    }

    /* 互動模式 */
    char line[MAX_LINE];

    printf("msh> ");
    fflush(stdout);

    while (fgets(line, MAX_LINE, stdin)) {
        trim_newline(line);

        /* 跳過空白行 */
        char *p = line;
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == '\0') {
            printf("msh> ");
            fflush(stdout);
            continue;
        }

        char *tokens[MAX_ARGS];
        int token_count = parse_line(line, tokens, MAX_ARGS);

        /* 按 | 分割成多個命令 */
        Command cmds[MAX_CMD];
        int cmd_count = 0;
        int start = 0;

        for (int i = 0; i <= token_count; i++) {
            if (i == token_count || strcmp(tokens[i], "|") == 0) {
                int n = parse_redirect(tokens + start, i - start, &cmds[cmd_count]);
                if (n > 0) cmd_count++;
                start = i + 1;
            }
        }

        if (cmd_count > 0)
            execute_pipeline(cmds, cmd_count);

        /* 清理背景行程 */
        job_cleanup();

        printf("msh> ");
        fflush(stdout);
    }

    printf("\n");
    return 0;
}
