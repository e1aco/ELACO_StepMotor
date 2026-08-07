/*****************************************************************************
 * @文件: ela_can_queue.h
 * @作者: cl
 * @日期: 2026-08-06
 * @版本: v1.0.0
 * @说明: CAN 帧命令队列，固定帧长循环队列
 ****************************************************************************/

#ifndef ELA_CAN_QUEUE_H

/* ==== 常量定义 ==== */
#define ELA_CAN_QUEUE_H

#define QUEUE_SIZE  48
#define CAN_LENGTH  8
#define QUEUE_NULL  0x00
#define TRUE        1
#define FALSE       0

/* ==== 类型定义 ==== */
/********
 * @说明: CAN 命令队列结构体
 ********/
typedef struct QUEUE
{
    unsigned char _queue[QUEUE_SIZE][CAN_LENGTH];
    signed char front;
    signed char rear;
} QUEUE_T;

/* ==== 全局实例 ==== */
extern QUEUE_T g_can_queue_st;
extern unsigned char g_all_can_count;

/* ==== 接口 ==== */




unsigned char USR_CanQueue_Insert(QUEUE_T *me, unsigned char *value);
unsigned char USR_CanQueue_Delete(QUEUE_T *me);
void         *USR_CanQueue_First(QUEUE_T *me);
unsigned char USR_CanQueue_IsFull(QUEUE_T *me);
unsigned char USR_CanQueue_IsEmpty(QUEUE_T *me);
void          USR_CanQueue_Init(QUEUE_T *me);

#endif






