/****
 * @ 文件: ela_queue.h
 * @ 作者: ELA
 * @ 日期: 2026-07-21
 * @ 版本: 2.0.0
 * @ 说明: 通用循环队列模块，零硬件依赖，可直接复用
 ********/
#ifndef ELA_QUEUE_H
#define ELA_QUEUE_H

#include <stdint.h>

/* module hlp start */

#define QUEUE_SIZE      48
#define QUEUE_ELEM_SIZE 8
#define QUEUE_OK        0
#define QUEUE_FULL      1
#define QUEUE_NULL_PTR  ((void *)0)

/* module hlp end */

typedef struct {
    uint8_t _queue[QUEUE_SIZE][QUEUE_ELEM_SIZE];
    int8_t front;
    int8_t rear;
} QUEUE_T;

extern QUEUE_T g_queue_st;

uint8_t ela_queue_insert(QUEUE_T *me, uint8_t *value);
uint8_t ela_queue_delete(QUEUE_T *me);
void   *ela_queue_first(QUEUE_T *me);
uint8_t ela_queue_is_full(QUEUE_T *me);
uint8_t ela_queue_is_empty(QUEUE_T *me);
void    ela_queue_init(QUEUE_T *me);

#endif
