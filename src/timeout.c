//
// Created by htc on 15-6-26.
//

#include "priority_queue.h"
#include "common.h"

static struct timeout_task {
  int id;
  struct timespec timeout;
  void (*handler)(void* data);
  void *data;
} *tasks = NULL;

static int global_task_id;
static pri_queue queue;

void timeout_init() {
  queue = priq_new(16);
  global_task_id = 1;
}

int timeout_register(long long msec, void (*handler)(void* data), void* data) {
  struct timeout_task* task = malloc(sizeof(struct timeout_task));

  task->id = global_task_id++;

  task->timeout = now;
  long long nsec = msec * 1000 * 1000 + task->timeout.tv_nsec;
  while (nsec >= 1000 * 1000 * 1000LL) {
    nsec -= 1000 * 1000 * 1000LL;
    task->timeout.tv_sec++;
  }
  task->timeout.tv_nsec = nsec;

  task->handler = handler;
  task->data = data;

  priq_push(queue, task, task->timeout.tv_sec * 1000 + task->timeout.tv_nsec / 1000 / 1000);

  return task->id;
}

static int time_compare(struct timespec *a, struct timespec *b) {
  if (a->tv_sec < b->tv_sec) {
    return 1;
  }
  if (a->tv_sec > b->tv_sec) {
    return 0;
  }
  return a->tv_nsec < b->tv_nsec;
}

int timeout_dispatch() {
  static struct timeout_task* task_buf[65536];
  struct timeout_task **task_top = task_buf, **task = task_buf;

  while (priq_size(queue)) {
    struct timeout_task *t = priq_top(queue, NULL);
    if (time_compare(&now, &t->timeout)) {
      break;
    }

    priq_pop(queue, NULL);
    *task_top++ = t;
  }

  while (task < task_top) {
    if ((*task)->handler) {
      (*task)->handler((*task)->data);
    }
    free(*task++);
  }

  return 0;
}

int time_diff(struct timespec* x, struct timespec* y, struct timespec* result) {
  static const long long E9 = 1000000000;
  if (x->tv_nsec < y->tv_nsec) {
    long long nsec = (y->tv_nsec - x->tv_nsec) / E9 + 1;
    y->tv_nsec -= E9 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_nsec - y->tv_nsec > E9) {
    long long nsec = (x->tv_nsec - y->tv_nsec) / E9;
    y->tv_nsec += E9 * nsec;
    y->tv_sec -= nsec;
  }

  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_nsec = x->tv_nsec - y->tv_nsec;

  return x->tv_sec < y->tv_sec;
}


