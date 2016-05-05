#include <fcntl.h>
#include <string.h>

#include "common.h"

int create_tun(char* name) {
  char path[256];
  strcpy(path, TUN_DEVICE);
  strcat(path, name);

  int fd = open(path, O_RDWR);
  if (fd < 0) {
    fatal("Cannot open tun device");
  }

  int flags = fcntl(fd, F_GETFL, 0);
  int ret = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  if (ret < 0) {
    fatal("fcntl");
  }

  return fd;
}

void setup_tun(char* name, char* src, char* dst, int mtu) {
  char cmd[128];

  sprintf(cmd, "ifconfig %s %s %s up mtu %d", name, src, dst, mtu);
  system(cmd);
}
