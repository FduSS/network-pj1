#include "common.h"
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


int main(int argc, char* argv[]) {
  if (parse_arg(argc, argv)) {
    printf("Can't parse arguments!\n");
    exit(-1);
  }

  timeout_init();
  get_now();
  srand(now.tv_sec);

  int tunA, tunB;
  tunA = create_tun(A_NAME);
  tunB = create_tun(B_NAME);

  if (tunA < 0 || tunB < 0) {
    return -1;
  }

  setup_tun(tunA, A_NAME, A_SRC, A_DST, 1500);
  setup_tun(tunB, B_NAME, B_DST, B_SRC, 1500);
  printf("Successfully init tun interfaces.\n");

  struct task tasks[2];
  setup_task(tasks, "A->B", tunA, tunB,
             inet_addr(A_SRC), inet_addr(A_DST), inet_addr(B_SRC), inet_addr(B_DST));
  setup_task(tasks + 1, "B->A", tunB, tunA,
             inet_addr(B_DST), inet_addr(B_SRC), inet_addr(A_DST), inet_addr(A_SRC));

  struct timespec last_sec = now;

  for (int i = 0; i < 2; ++i) {
    task_transfer(tasks + i);
  }

  while (1) {
    poll_read(tasks, 2);

    struct timespec interval;
    time_diff(&now, &last_sec, &interval);
    int print_stat = interval.tv_sec > 0;
    if (print_stat) {
      last_sec = now;
      printf("\n");
    }

    timeout_dispatch();
    get_now();
    for (int i = 0; i < 2; ++i) {
      task_update(tasks + i, print_stat);
    }
  }
}