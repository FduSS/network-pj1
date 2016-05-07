//
// Created by htc on 5/5/16.
//

#ifndef TRANSFER_TASK_H
#define TRANSFER_TASK_H

#include <stdint.h>

struct speed_stat {
    int packet_count;
    long data_count;

};

struct task {
    int in_fd, out_fd;
    uint32_t src, dst, nat_src, nat_dst;
};

extern void task_transfer(struct task* task);
extern void setup_task(struct task* task);

#endif //TRANSFER_TASK_H
