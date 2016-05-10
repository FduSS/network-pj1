//
// Created by htc on 15-6-26.
//

#ifndef TRANSFER_TIMEOUT_H
#define TRANSFER_TIMEOUT_H

#include <stdint.h>

void timeout_init();
int timeout_register(long long msec, void (*handler)(void* data), void* data);
int timeout_dispatch();
int time_diff(struct timespec* x, struct timespec* y, struct timespec* result);

#endif
