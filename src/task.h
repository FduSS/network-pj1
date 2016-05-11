//
// Created by htc on 5/5/16.
//

#ifndef TRANSFER_TASK_H
#define TRANSFER_TASK_H

#include <stdint.h>

struct speed_stat {
  int packet_count;
  long data_count;
  long long token, token_per_sec, token_brust;
  struct timespec last_update;
};

struct task {
  char* name;
  int fd_in, fd_out;
  uint32_t src, dst, nat_src, nat_dst;
  struct speed_stat stat;
};

void task_transfer(struct task* task);
void setup_task(struct task* task, char* name, int fd_in, int fd_out,
                uint32_t src, uint32_t dst, uint32_t nat_src, uint32_t nat_dst);
void task_update(struct task* task, int print_stat);

#endif //TRANSFER_TASK_H
