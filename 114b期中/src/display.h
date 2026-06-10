#ifndef DISPLAY_H
#define DISPLAY_H

#include "procfs.h"

/* ANSI escape codes */
#define ESC         "\033["
#define CLEAR       ESC "2J"
#define HOME        ESC "H"
#define HIDE_CURSOR ESC "?25l"
#define SHOW_CURSOR ESC "?25h"
#define BOLD        ESC "1m"
#define RESET       ESC "0m"
#define REV         ESC "7m"     /* reverse video */

/* 顏色 */
#define RED     ESC "31m"
#define GREEN   ESC "32m"
#define YELLOW  ESC "33m"
#define CYAN    ESC "36m"
#define WHITE   ESC "37m"
#define GRAY    ESC "90m"

/* 初始化終端機 */
void display_init(void);

/* 恢復終端機 */
void display_restore(void);

/* 顯示表頭（系統資訊）*/
void display_header(const MemInfo *mi, const CpuTime *ct,
                    int process_count, int sort_mode);

/* 顯示行程列表 */
void display_processes(const ProcessInfo *procs, int count,
                       int max_rows, int sort_mode);

/* 顯示表尾（操作提示）*/
void display_footer(int sort_mode);

/* 移動游標到 */
void display_goto(int row, int col);

/* 取得終端機尺寸 */
int get_term_rows(void);
int get_term_cols(void);

#endif
