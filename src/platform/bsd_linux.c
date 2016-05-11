#include "../common.h"

ssize_t read_tun(int fd, char* data, size_t len) {
  return read(fd, data, len);
}

ssize_t write_tun(int fd, char* data, size_t len) {
  return write(fd, data, len);
}

void poll_read(struct task* tasks, int n) {
  int nfds = 0;
  fd_set rfds;
  FD_ZERO(&rfds);

  for (int i = 0; i < n; ++i) {
    FD_SET(tasks[i].fd_in, &rfds);
    if (tasks[i].fd_in + 1 > nfds) {
      nfds = tasks[i].fd_in + 1;
    }
  }

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 1;

  int ret = select(nfds, &rfds, NULL, NULL, &tv);
  if (ret < 0) {
    fatal("select");
  }

  get_now();
  for (int i = 0; i < n; ++i) {
    if (FD_ISSET(tasks[i].fd_in, &rfds)) {
      task_transfer(tasks + i);
    }
  }
}
