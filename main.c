#include <stdio.h>
#include <netinet/in.h>
#include <linux/if_tun.h>
#include <linux/if.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define TUN_DEVICE "/dev/net/tun"
#define A_NAME "tunA"
#define A_SRC "172.19.0.2"
#define A_DST "172.19.0.1"
#define B_NAME "tunB"
#define B_SRC "172.20.0.2"
#define B_DST "172.20.0.1"
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

void setup_task(struct task* task) {
}

int task_check_packet(struct task* task, char* packet, ssize_t len) {
  if (len < 20) {
    return -1;
  }

  uint32_t src = *(uint32_t*)(packet + 12);
  uint32_t dst = *(uint32_t*)(packet + 16);
  if (src != task->src || dst != task->dst) {
    return -1;
  }

  return 0;
}

uint16_t checksum(void* data, ssize_t len, uint16_t init) {
  uint8_t* p = (uint8_t*) data;
  uint8_t* end = p + len;
  uint32_t s = init;

  while (p < end) {
    s += *p;
    if (p+1 < end) {
      s += ((uint32_t)*(p+1)) << 8;
    }
    p += 2;
  }

  while (s > 0xffff) {
    s = (s & 0xffff) + (s >> 16);
  }

  return ~s & 0xffff;
}

void modify_checksum(char* data, ssize_t len, ssize_t ck_offset, uint16_t init) {
  uint16_t* ck = (uint16_t*)(data + ck_offset);
  *ck = 0;
  *ck = checksum(data, len, init);
}

int task_nat_packet(struct task* task, char* packet, ssize_t len) {
  *(uint32_t*)(packet + 12) = task->nat_src;
  *(uint32_t*)(packet + 16) = task->nat_dst;
  ssize_t header_len = ((uint8_t)packet[0] & 0xf) * 4;
  modify_checksum(packet, header_len, 10, 0);

  char* data = packet + header_len;
  ssize_t data_len = len - header_len;

  uint8_t type = (uint8_t) packet[9];
  struct {
    uint32_t src, dst;
    uint8_t zero, type;
    uint16_t len;
  } pseudo = {
      .src = task->nat_src,
      .dst = task->nat_dst,
      .zero = 0,
      .type = type,
      .len = htons(data_len)
  };

  switch (type) {
    case IPPROTO_TCP:
      //printf("TCP\n");
      if (data_len < 18) {
        return -1;
      }
      modify_checksum(data, data_len, 16,
                      ~checksum(&pseudo, sizeof(pseudo), 0));
      break;

    case IPPROTO_UDP:
      //printf("UDP\n");
      if (data_len < 8) {
        return -1;
      }
      modify_checksum(data, data_len, 6,
                      ~checksum(&pseudo, sizeof(pseudo), 0));
      break;

    case IPPROTO_ICMP:
      //printf("ICMP\n");
      modify_checksum(data, data_len, 2, 0);
      break;

    default:
      break;
  }

  return 0;
}

void task_transfer(struct task* task) {
  char packet[MTU];

  while (1) {
    ssize_t packet_len = read(task->in_fd, packet, MTU);
    if (packet_len < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return;
      }
      fatal("read");
    }

    if (task_check_packet(task, packet, packet_len)) {
      continue;
    }

    if (task_nat_packet(task, packet, packet_len)) {
      continue;
    }

    write(task->out_fd, packet, packet_len);
  }
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

int main() {
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
  }
}