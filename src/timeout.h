//
// Created by htc on 15-6-26.
//

#ifndef TRANSFER_TIMEOUT_H
#define TRANSFER_TIMEOUT_H

#include <sys/time.h>
#include <stdint.h>

extern void timeout_init();
extern int timeout_register(long long msec, void (*handler)(void* data), void* data);
extern int timeout_dispatch();

#endif
