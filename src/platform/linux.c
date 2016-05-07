#include <fcntl.h>
#include <string.h>
#include <netinet/ip.h>
#include <sys/ioctl.h>
#include <linux/if_tun.h>
#include <linux/if.h>

#include "../common.h"

int create_tun(char* name) {
  int fd = open(TUN_DEVICE, O_RDWR);
  if (fd < 0) {
    fatal("Cannot open tun device");
  }

  int flags = fcntl(fd, F_GETFL, 0);
  int ret = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  if (ret < 0) {
    fatal("fcntl");
  }

  struct ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_TUN | IFF_NO_PI | IFF_MULTI_QUEUE;
  strcpy(ifr.ifr_name, name);
  ret = ioctl(fd, TUNSETIFF, &ifr);
  if (ret < 0) {
    fatal("ioctl");
  }

  return fd;
}

void setup_tun(char* name, char* src, char* dst, int mtu) {
  char cmd[128];

  sprintf(cmd, "ip link set %s up mtu %d", name, mtu);
  system(cmd);
  sprintf(cmd, "ip addr add %s/32 dev %s", src, name);
  system(cmd);
  sprintf(cmd, "ip route add %s/32 via %s dev %s", dst, src, name);
  system(cmd);
}

