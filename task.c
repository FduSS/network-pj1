#include <errno.h>
#include <unistd.h>
#include "common.h"
#include "task.h"

void setup_task(struct task* task) {
}

static int task_check_packet(struct task* task, char* packet, ssize_t len) {
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

static uint16_t checksum(void* data, ssize_t len, uint16_t init) {
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

  return (uint16_t) (~s & 0xffff);
}

static void modify_checksum(char* data, ssize_t len, ssize_t ck_offset, uint16_t init) {
  uint16_t* ck = (uint16_t*)(data + ck_offset);
  *ck = 0;
  *ck = checksum(data, len, init);
}

static int task_nat_packet(struct task* task, char* packet, ssize_t len) {
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
