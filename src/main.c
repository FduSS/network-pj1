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

//void do_poll(struct task* tasks, int n) {
//  int nfds = 0;
//  fd_set rfds;
//  FD_ZERO(&rfds);
//
//  for (int i = 0; i < n; ++i) {
//    FD_SET(tasks[i].in_fd, &rfds);
//    if (tasks[i].in_fd + 1 > nfds) {
//      nfds = tasks[i].in_fd + 1;
//    }
//  }
//
//#ifdef CYGWIN
//  TIMEVAL tv;
//#else
//  struct timeval tv;
//#endif
//  tv.tv_sec = 0;
//  tv.tv_usec = 1;
//
//  int ret = select(nfds, &rfds, NULL, NULL, &tv);
//  if (ret < 0) {
//    fatal("select");
//  }
//
//  for (int i = 0; i < n; ++i) {
//    if (FD_ISSET(tasks[i].in_fd, &rfds)) {
//      task_transfer(tasks + i);
//    }
//  }
//}

int main(int argc, char* argv[]) {
  if (parse_arg(argc, argv)) {
    printf("Can't parse arguments!\n");
    exit(-1);
  }
  timeout_init();
  get_now();
  srand(now.tv_sec);

  struct tun_device tunA, tunB;

  if (create_tun(&tunA, A_NAME) < 0 || create_tun(&tunB, B_NAME) < 0) {
    return -1;
  }

  printf("Successfully init tun interfaces.\n");

  struct task tasks[2];
  setup_task(tasks, "A->B", &tunA, &tunB,
             inet_addr(A_SRC), inet_addr(A_DST), inet_addr(B_SRC), inet_addr(B_DST));
  setup_task(tasks + 1, "B->A", &tunB, &tunA,
             inet_addr(B_DST), inet_addr(B_SRC), inet_addr(A_DST), inet_addr(A_SRC));

  while (1) {
    poll_read(tasks, 2);
    timeout_dispatch();
    for (int i = 0; i < 2; ++i) {
      task_update(tasks + i);
    }
  }
}