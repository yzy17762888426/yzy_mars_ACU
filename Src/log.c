#include "log.h"
#include "display_comm.h"
#include "get_maf.h"
#include <stdio.h>
#include <string.h>

#define NEX_END         "\xff\xff\xff"
#define LOG_CH          6       // slt0 ~ slt5
#define MAX_PER_CH      28      // 每通道最大条数
#define LOG_BUF_SIZE    253     // 每通道缓冲(28行 × 9字节 + 1)
#define FLUSH_INTERVAL  10      // 每 10 次调用刷新一次(1 秒)
#define LINE_W          9       // 每行固定 9 字符: "999 12345"

static uint8_t  log_started = 0;
static uint8_t  log_cur = 0;
static uint16_t log_seq = 0;
static uint8_t  log_count[LOG_CH];
static char     log_buf[LOG_CH][LOG_BUF_SIZE];
static uint16_t log_len[LOG_CH];
static uint8_t  log_flush_cnt = 0;

static void Log_AddLine(const char *text)
{
    if (log_count[log_cur] >= MAX_PER_CH)
    {
        printf("slt%d.txt=\"%s\"" NEX_END, log_cur, log_buf[log_cur]);
        HAL_Delay(10);

        log_cur = (log_cur + 1) % LOG_CH;
        log_buf[log_cur][0] = '\0';
        log_len[log_cur] = 0;
        log_count[log_cur] = 0;
        printf("slt%d.txt=\"\"" NEX_END, log_cur);
        HAL_Delay(10);
        log_flush_cnt = FLUSH_INTERVAL - 1;
    }

    memcpy(&log_buf[log_cur][log_len[log_cur]], text, LINE_W);
    log_len[log_cur] += LINE_W;
    log_buf[log_cur][log_len[log_cur]] = '\0';
    log_count[log_cur]++;
}

void Log_Init(void)
{
    log_started = 0;
    log_cur = 0;
    log_seq = 0;
    log_flush_cnt = 0;
    memset(log_count, 0, sizeof(log_count));
    memset(log_len, 0, sizeof(log_len));
}

void Log_Task(void)
{
    if (!log_en)
    {
        log_started = 0;
        return;
    }

    if (!log_started)
    {
        log_started = 1;
        log_cur = 0;
        log_seq = 0;
        log_flush_cnt = 0;
        for (int i = 0; i < LOG_CH; i++)
        {
            log_buf[i][0] = '\0';
            log_len[i] = 0;
            log_count[i] = 0;
        }
    }

    /* 序号 0~999 循环, 格式 "999 12345" 固定 9 字符 */
    if (++log_seq > 999) log_seq = 0;
    char line[LINE_W + 1];
    int n = snprintf(line, sizeof(line), "%3d %-5d", log_seq, GetFreqShow());
    if (n > LINE_W) n = LINE_W;
    while (n < LINE_W) line[n++] = ' ';
    line[LINE_W] = '\0';

    Log_AddLine(line);

    if (++log_flush_cnt >= FLUSH_INTERVAL)
    {
        log_flush_cnt = 0;
        printf("slt%d.txt=\"%s\"" NEX_END, log_cur, log_buf[log_cur]);
        HAL_Delay(10);
    }
}
