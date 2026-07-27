/********
 * @ 文件: ela_can_queue.h
 * @ 作者: ELACO
 * @ 日期: 2026-07-17
 * @ 版本: 1.0.0
 * @ 说明: CAN 帧命令队列，固定帧长循环队列
 ********/

#ifndef ELA_CAN_QUEUE_H
#define ELA_CAN_QUEUE_H

#define QUEUE_SIZE  48
#define CAN_LENGTH  8
#define QUEUE_NULL  0x00
#define TRUE        1
#define FALSE       0

/********
 * @ 说明: CAN 命令队列结构体
 ********/
typedef struct QUEUE
{
    unsigned char _queue[QUEUE_SIZE][CAN_LENGTH];
    signed char front;
    signed char rear;
} QUEUE_T;

extern QUEUE_T g_can_queue_st;
extern unsigned char g_all_can_count;

unsigned char ela_can_queue_insert(QUEUE_T *me, unsigned char *value);
unsigned char ela_can_queue_delete(QUEUE_T *me);
void         *ela_can_queue_first(QUEUE_T *me);
unsigned char ela_can_queue_is_full(QUEUE_T *me);
unsigned char ela_can_queue_is_empty(QUEUE_T *me);
void          ela_can_queue_init(QUEUE_T *me);

#endif

