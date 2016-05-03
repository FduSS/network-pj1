#include <stdio.h>
#include <netinet/in.h>
#include <linux/if_tun.h>
#include <linux/if.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#define TUN_DEVICE "/dev/net/tun"
#define A_NAME "tunA"
#define A_SRC "172.19.0.2"
#define A_DST "172.19.0.1"
#define B_NAME "tunB"
#define B_SRC "172.20.0.1"
#define B_DST "172.20.0.2"
#define MTU 1500

void fatal(char* error) {
  perror(error);
  exit(-1);
}

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

struct task {
  int in_fd, out_fd;
  uint32_t src, dst, nat_src, nat_dst;
};

int main() {
  int tun_a = create_tun(A_NAME);
  int tun_b = create_tun(B_NAME);
  setup_tun(A_NAME, A_SRC, A_DST, MTU);
  setup_tun(B_NAME, B_SRC, B_DST, MTU);

  printf("Successfully init tun interfaces.\n");

  struct task task_a = {
      .in_fd = tun_a,
      .out_fd = tun_b,
      .src = inet_addr(A_SRC),
      .dst = inet_addr(A_DST),
      .nat_src = inet_addr(B_SRC),
      .nat_dst = inet_addr(B_DST)
  };

  struct task task_b = {
      .in_fd = tun_b,
      .out_fd = tun_a,
      .src = inet_addr(B_DST),
      .dst = inet_addr(B_SRC),
      .nat_src = inet_addr(A_DST),
      .nat_dst = inet_addr(A_SRC)
  };
}