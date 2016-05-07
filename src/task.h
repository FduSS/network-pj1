//
// Created by htc on 5/5/16.
//

#ifndef TRANSFER_TASK_H
#define TRANSFER_TASK_H

#include <stdint.h>

struct speed_stat {
    int sec_packet_count;
    long sec_data_count;
    long long token, token_per_sec, token_brust;
    struct timespec last_update, last_sec;
};

struct task {
    char* name;
    int in_fd, out_fd;
    uint32_t src, dst, nat_src, nat_dst;
    struct speed_stat stat;
};

extern void task_transfer(struct task* task);
extern void setup_task(struct task* task, char* name, int in_fd, int out_fd,
                       uint32_t src, uint32_t dst, uint32_t nat_src, uint32_t nat_dst);
extern void task_update(struct task* task);

#endif //TRANSFER_TASK_H
