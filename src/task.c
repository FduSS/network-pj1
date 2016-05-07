#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include "common.h"
#include "task.h"
#include "config.h"
#include "timeout.h"

struct pending_write {
    int fd;
    size_t len;
    char packet[0];
};

void setup_task(struct task* task, char* name, int in_fd, int out_fd,
                uint32_t src, uint32_t dst, uint32_t nat_src, uint32_t nat_dst) {

  struct timespec now;
  get_now(&now);

  long brust = config_speed_limit / 32;
  *task = (struct task) {
      .name = name,
      .in_fd = in_fd,
      .out_fd = out_fd,
      .src = src,
      .dst = dst,
      .nat_src = nat_src,
      .nat_dst = nat_dst,
      .stat = {
          .sec_data_count = 0,
          .sec_packet_count = 0,
          .token = brust,
          .token_brust = brust,
          .token_per_sec = config_speed_limit,
          .last_update = now,
          .last_sec = now,
      }
  };
}

static int task_drop_packet(struct task* task, char* packet, ssize_t len) {
  if (len < 20) {
    return -1;
  }

  uint32_t src = *(uint32_t*)(packet + 12);
  uint32_t dst = *(uint32_t*)(packet + 16);
  if (src != task->src || dst != task->dst) {
    // unexcepted packet
    return -1;
  }

  if ((double)rand() / RAND_MAX < config_drop_rate) {
    // drop packet
    return 1;
  }

  struct speed_stat* stat = &task->stat;

  if (stat->token_per_sec == 0) {
    // speed limit not enabled
    return 0;
  }

  stat->token -= len;
  if (stat->token < 0) {
    // limit the speed
    printf("drop by speed limit.\n");
    stat->token = 0;
    return 1;
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

static void task_write(void* data) {
  struct pending_write* w = (struct pending_write*) data;
  write(w->fd, w->packet, w->len);
  free(w);
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

    if (task_drop_packet(task, packet, packet_len)) {
      continue;
    }

    if (task_nat_packet(task, packet, packet_len)) {
      continue;
    }

    task->stat.sec_packet_count ++;
    task->stat.sec_data_count += packet_len;

    struct pending_write* w = malloc(sizeof(struct pending_write) + packet_len);
    if (w == NULL) {
      printf("Out of memory!\n");
      exit(-1);
    }
    w->fd = task->out_fd;
    w->len = packet_len;
    memcpy(w->packet, packet, packet_len);
    long int delay = lround(config_delay * (1 + config_delay_trashing * (2.0 * rand()/RAND_MAX - 1)));
    timeout_register(delay, task_write, w);
  }
}

void task_print_stat(struct task* task) {
  printf("%s: packet: %d transfer: ", task->name, task->stat.sec_packet_count);

  double size = task->stat.sec_data_count * 8;
  if (size < 1024) {
    printf("%.2f bps\n", size);
    return;
  }
  size /= 1024;
  if (size < 1024) {
    printf("%.2f Kbps\n", size);
    return;
  }
  size /= 1024;
  if (size < 1024) {
    printf("%.2f Mbps\n", size);
    return;
  }
  size /= 1024;
  printf("%.2f Gbps\n", size);
}

void task_update(struct task* task) {
  struct speed_stat* stat = &task->stat;
  struct timespec now, diff;
  get_now(&now);

  if (stat->sec_packet_count == 0) {
    stat->sec_data_count = 0;
    stat->last_sec = now;
  } else {
    time_diff(&now, &stat->last_sec, &diff);
    if (diff.tv_sec > 0) {
      task_print_stat(task);
      stat->last_sec = now;
      stat->sec_packet_count = 0;
      stat->sec_data_count = 0;
    }
  }

  if (stat->token_per_sec == 0) {
    return;
  }

  time_diff(&now, &stat->last_update, &diff);
  long long rem = diff.tv_nsec * stat->token_per_sec / 1000000000;
  stat->token += rem;
  if (stat->token > stat->token_brust) {
    stat->token = stat->token_brust;
  }

  stat->last_update = now;
}
