#ifndef _ELA_QUEUE_H_
#define _ELA_QUEUE_H_

#define QUEUE_SIZE  48  // 队列最大元素个数
#define CAN_LENGTH      8   // 每条命令的长度（包含校验位）
#define NULL1       0x00
#define FALSE       1
#define TRUE        0

typedef struct QUEUE
{
    unsigned char _queue[QUEUE_SIZE][CAN_LENGTH];
    signed char front;  // 队列头指针
    signed char rear;   // 队列尾指针
} QUEUE_T;

extern QUEUE_T gov_can_queue_st;
extern unsigned char gov_can_count;

unsigned char Queue_Insert(QUEUE_T *me, unsigned char *value);
unsigned char Queue_Delete(QUEUE_T *me);
void         *Queue_First(QUEUE_T *me);
unsigned char Queue_IsFull(QUEUE_T *me);
unsigned char Queue_IsEmpty(QUEUE_T *me);
void          Queue_Init(QUEUE_T *me);


#endif
