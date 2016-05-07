#include <unistd.h>
#include <errno.h>

#include "common.h"
#include "task.h"
#include "config.h"
#include "timeout.h"

#define A_SRC "172.19.0.2"
#define A_DST "172.19.0.1"
#define B_SRC "172.20.0.2"
#define B_DST "172.20.0.1"

void fatal(char* error) {
  perror(error);
  exit(-1);
}

void do_poll(struct task* tasks, int n) {
  int nfds = 0;
  fd_set rfds;
  FD_ZERO(&rfds);

  for (int i = 0; i < n; ++i) {
    FD_SET(tasks[i].in_fd, &rfds);
    if (tasks[i].in_fd + 1 > nfds) {
      nfds = tasks[i].in_fd + 1;
    }
  }

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 1;

  int ret = select(nfds, &rfds, NULL, NULL, &tv);
  if (ret < 0) {
    fatal("select");
  }

  for (int i = 0; i < n; ++i) {
    if (FD_ISSET(tasks[i].in_fd, &rfds)) {
      task_transfer(tasks + i);
    }
  }
}

int main(int argc, char* argv[]) {
  if (parse_arg(argc, argv)) {
    printf("Can't parse arguments!\n");
    exit(-1);
  }
  timeout_init();
  srand(time(NULL));

  int tun_a = create_tun(A_NAME);
  int tun_b = create_tun(B_NAME);
  setup_tun(A_NAME, A_SRC, A_DST, MTU);
  setup_tun(B_NAME, B_DST, B_SRC, MTU);

  printf("Successfully init tun interfaces.\n");

  struct task tasks[2];
  tasks[0] = (struct task) {
      .in_fd = tun_a,
      .out_fd = tun_b,
      .src = inet_addr(A_SRC),
      .dst = inet_addr(A_DST),
      .nat_src = inet_addr(B_SRC),
      .nat_dst = inet_addr(B_DST)
  };

  tasks[1] = (struct task) {
      .in_fd = tun_b,
      .out_fd = tun_a,
      .src = inet_addr(B_DST),
      .dst = inet_addr(B_SRC),
      .nat_src = inet_addr(A_DST),
      .nat_dst = inet_addr(A_SRC)
  };

  for (int i = 0; i < 2; ++i) {
    setup_task(tasks + i);
  }

  while (1) {
    do_poll(tasks, 2);
    timeout_dispatch();
  }
}