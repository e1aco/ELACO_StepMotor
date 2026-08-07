/*****************************************************************************
 * @文件: ela_can_queue.c
 * @作者: cl
 * @日期: 2026-08-06
 * @版本: v1.0.0
 * @说明: CAN 帧命令循环队列实现
 ****************************************************************************/

#include "ela_can_queue.h"

/* ==== 全局实例 ==== */
QUEUE_T g_can_queue_st;
unsigned char g_all_can_count = 0;

/* ==== 接口实现 ==== */
/********
 * @输入: me: 队列结构体指针; value: 待插入的数据指针
 * @输出: TRUE(0) 成功, FALSE(1) 队列满
 * @说明: 将一个命令数据插入到队列尾部
 ********/
unsigned char USR_CanQueue_Insert(QUEUE_T *me, unsigned char *value)
{
    unsigned char i;

    if (USR_CanQueue_IsFull(me))
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

/********
 * @输入: me: 队列结构体指针
 * @输出: TRUE(0) 成功, FALSE(1) 队列空
 * @说明: 从队列头部删除一个命令数据
 ********/
unsigned char USR_CanQueue_Delete(QUEUE_T *me)
{
    if (USR_CanQueue_IsEmpty(me))
    {
        return FALSE;
    }

    if (me->front == me->rear)
    {
        me->front = -1;
    }
    else
    {
        me->front = (me->front + 1) % QUEUE_SIZE;
    }

    return TRUE;
}

/********
 * @输入: me: 队列结构体指针
 * @输出: 队列头部元素地址，队列空返回 NULL
 * @说明: 获取队列头部第一个命令数据的地址
 ********/
void *USR_CanQueue_First(QUEUE_T *me)
{
    if (USR_CanQueue_IsEmpty(me))
    {
        return (void *)QUEUE_NULL;
    }
    else
    {
        return &(me->_queue[me->front]);
    }
}

/********
 * @输入: me: 队列结构体指针
 * @输出: TRUE(0) 队列满, FALSE(1) 未满
 * @说明: 判断队列是否已满
 ********/
unsigned char USR_CanQueue_IsFull(QUEUE_T *me)
{
    return me->front == (me->rear + 1) % QUEUE_SIZE;
}

/********
 * @输入: me: 队列结构体指针
 * @输出: TRUE(0) 队列空, FALSE(1) 非空
 * @说明: 判断队列是否为空
 ********/
unsigned char USR_CanQueue_IsEmpty(QUEUE_T *me)
{
    return me->front == -1;
}

/********
 * @输入: me: 队列结构体指针
 * @说明: 初始化队列，将 front 和 rear 都设置为 -1
 ********/
void USR_CanQueue_Init(QUEUE_T *me)
{
    me->front = -1;
    me->rear = -1;
}






