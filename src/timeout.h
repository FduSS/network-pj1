//
// Created by htc on 15-6-26.
//

#ifndef TRANSFER_TIMEOUT_H
#define TRANSFER_TIMEOUT_H

#include <stdint.h>

extern void timeout_init();
extern int timeout_register(long long msec, void (*handler)(void* data), void* data);
extern int timeout_dispatch();
extern int time_diff(struct timespec* x, struct timespec* y, struct timespec* result);

#endif
