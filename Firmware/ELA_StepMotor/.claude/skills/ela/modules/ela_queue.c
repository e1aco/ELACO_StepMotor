/****
 * @ 文件: ela_queue.c
 * @ 作者: ELA
 * @ 日期: 2026-07-21
 * @ 版本: 2.0.0
 * @ 说明: 通用循环队列模块，零硬件依赖，可直接复用
 ********/
#include "ela_queue.h"

/* module hlp start */

/* module hlp end */
//----------------------------------------------------------------------------------
/* module drv start */

/* module drv end */
//----------------------------------------------------------------------------------
/* module usr start */

QUEUE_T g_queue_st;
uint8_t g_queue_count = 0;

/****
 * @ 输入: me: 队列结构体指针;
 *        value: 待插入的数据指针
 * @ 输出: uint8_t: 成功返回 QUEUE_OK(0)，队列满返回 QUEUE_FULL(1)
 * @ 说明: 将一个命令数据插入到队列尾部，支持循环队列模式
 ********/
uint8_t ela_queue_insert(QUEUE_T *me, uint8_t *value)
{
    uint8_t i;

    if (ela_queue_is_full(me))
    {
        return QUEUE_FULL;
    }

    me->rear = (me->rear + 1) % QUEUE_SIZE;

    for (i = 0; i < QUEUE_ELEM_SIZE; i++)
    {
        (me->_queue)[me->rear][i] = value[i];
    }

    if (me->front == -1)
    {
        me->front = me->rear;
    }

    return QUEUE_OK;
}

/****
 * @ 输入: me: 队列结构体指针
 * @ 输出: uint8_t: 成功返回 QUEUE_OK(0)，队列空返回 QUEUE_FULL(1)
 * @ 说明: 从队列头部删除一个命令数据
 ********/
uint8_t ela_queue_delete(QUEUE_T *me)
{
    if (ela_queue_is_empty(me))
    {
        return QUEUE_FULL;
    }

    if (me->front == me->rear)
    {
        me->front = -1;
    }
    else
    {
        me->front = (me->front + 1) % QUEUE_SIZE;
    }

    return QUEUE_OK;
}

/****
 * @ 输入: me: 队列结构体指针
 * @ 输出: void *: 返回队列头部元素地址，队列空返回 QUEUE_NULL_PTR
 * @ 说明: 获取队列头部第一个命令数据的地址，
 *        用于读取但不移除数据
 ********/
void *ela_queue_first(QUEUE_T *me)
{
    if (ela_queue_is_empty(me))
    {
        return QUEUE_NULL_PTR;
    }
    else
    {
        return &(me->_queue[me->front]);
    }
}

/****
 * @ 输入: me: 队列结构体指针
 * @ 输出: uint8_t: 队列满返回 QUEUE_OK(0)，未满返回 QUEUE_FULL(1)
 * @ 说明: 判断队列是否已满，
 *        front 指向 rear 的下一个位置时表示队列满
 ********/
uint8_t ela_queue_is_full(QUEUE_T *me)
{
    return me->front == (me->rear + 1) % QUEUE_SIZE;
}

/****
 * @ 输入: me: 队列结构体指针
 * @ 输出: uint8_t: 队列空返回 QUEUE_OK(0)，非空返回 QUEUE_FULL(1)
 * @ 说明: 判断队列是否为空，front 为 -1 时表示队列空
 ********/
uint8_t ela_queue_is_empty(QUEUE_T *me)
{
    return me->front == -1;
}

/****
 * @ 输入: me: 队列结构体指针
 * @ 输出: void
 * @ 说明: 初始化队列，将 front 和 rear 都设置为 -1
 ********/
void ela_queue_init(QUEUE_T *me)
{
    me->front = -1;
    me->rear = -1;
}

/* module usr end */
