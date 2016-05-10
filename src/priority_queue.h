//
// Created by htc on 15-6-26.
//

#ifndef TRANSFER_PRIORITY_QUEUE_H
#define TRANSFER_PRIORITY_QUEUE_H

#include "stdint.h"

typedef struct {
  void * data;
  int64_t pri;
} q_elem_t;

typedef struct {
  q_elem_t *buf; int n, alloc;
} pri_queue_t, *pri_queue;

#define priq_purge(q) (q)->n = 1
#define priq_size(q) ((q)->n - 1)

pri_queue priq_new(int size);
void priq_free(pri_queue q);
void priq_push(pri_queue q, void *data, int64_t pri);
void * priq_pop(pri_queue q, int64_t *pri);
void* priq_top(pri_queue q, int64_t *pri);


#endif
