#include "ela_queue.h"

QUEUE_T gov_can_queue_st;  // CAN 命令队列
unsigned char gov_can_count = 0;

/****
 * @ 原型: unsigned char Queue_Insert(QUEUE_T *me, unsigned char *value)
 * @ 输入: QUEUE_T *me: 队列结构体指针; unsigned char *value: 待插入的数据指针
 * @ 输出: unsigned char: 成功返回 TRUE(0)，队列满返回 FALSE(1)
 * @ 说明: 将一个命令数据插入到队列尾部，支持循环队列模式
 ********/
unsigned char Queue_Insert(QUEUE_T *me, unsigned char *value)
{
    unsigned char i;

    if (Queue_IsFull(me))
    {
        return FALSE;
    }

    me->rear = (me->rear + 1) % QUEUE_SIZE;

    for (i = 0; i < CAN_LENGTH; i++)
    {
        (me->_queue)[me->rear][i] = value[i];
    }

    if (me->front == -1)
    {
        me->front = me->rear;
    }

    return TRUE;
}

/****
 * @ 原型: unsigned char Queue_Delete(QUEUE_T *me)
 * @ 输入: QUEUE_T *me: 队列结构体指针
 * @ 输出: unsigned char: 成功返回 TRUE(0)，队列空返回 FALSE(1)
 * @ 说明: 从队列头部删除一个命令数据
 ********/
unsigned char Queue_Delete(QUEUE_T *me)
{
    if (Queue_IsEmpty(me))
    {
        return FALSE;
    }

    if (me->front == me->rear)
    {
        // 队列中只有一个元素，删除后队列为空
        me->front = -1;
    }
    else
    {
        me->front = (me->front + 1) % QUEUE_SIZE;
    }

    return TRUE;
}

/****
 * @ 原型: void *Queue_First(QUEUE_T *me)
 * @ 输入: QUEUE_T *me: 队列结构体指针
 * @ 输出: void *: 返回队列头部元素地址，队列空返回 NULL1(0x00)
 * @ 说明: 获取队列头部第一个命令数据的地址，用于读取但不移除数据
 ********/
void *Queue_First(QUEUE_T *me)
{
    if (Queue_IsEmpty(me))
    {
        return (void *)NULL1;
    }
    else
    {
        return &(me->_queue[me->front]);
    }
}

/****
 * @ 原型: unsigned char Queue_IsFull(QUEUE_T *me)
 * @ 输入: QUEUE_T *me: 队列结构体指针
 * @ 输出: unsigned char: 队列满返回 TRUE(0)，未满返回 FALSE(1)
 * @ 说明: 判断队列是否已满，front 指向 rear 的下一个位置时表示队列满
 ********/
unsigned char Queue_IsFull(QUEUE_T *me)
{
    return me->front == (me->rear + 1) % QUEUE_SIZE;
}

/****
 * @ 原型: unsigned char Queue_IsEmpty(QUEUE_T *me)
 * @ 输入: QUEUE_T *me: 队列结构体指针
 * @ 输出: unsigned char: 队列空返回 TRUE(0)，非空返回 FALSE(1)
 * @ 说明: 判断队列是否为空，front 为 -1 时表示队列空
 ********/
unsigned char Queue_IsEmpty(QUEUE_T *me)
{
    return me->front == -1;
}

/****
 * @ 原型: void Queue_Init(QUEUE_T *me)
 * @ 输入: QUEUE_T *me: 队列结构体指针
 * @ 输出: void
 * @ 说明: 初始化队列，将 front 和 rear 都设置为 -1
 ********/
void Queue_Init(QUEUE_T *me)
{
    me->front = -1;
    me->rear = -1;
}
