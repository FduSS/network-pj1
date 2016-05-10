#ifndef TRANSFER_COMMON_H
#define TRANSFER_COMMON_H

#define MTU 1500

#include <string.h>
#include <math.h>
#include <time.h>
#include "task.h"

#if defined(LINUX)
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#define TUN_DEVICE "/dev/net/tun"
#define A_NAME "tunA"
#define B_NAME "tunB"

#elif defined(OSX)
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#define TUN_DEVICE "/dev/"
#define A_NAME "tun8"
#define B_NAME "tun9"

#elif defined(MINGW)
#include <windows.h>
#include <stdio.h>
#define A_NAME "A"
#define B_NAME "B"

struct tun_device {
  HANDLE handle;
  struct task* read_task;
};

#else
#error "Unsupported system! This lab only support cygwin/Linux/MacOSX"
#endif

int create_tun(struct tun_device* tun, char* name);
void setup_tun(struct tun_device* tun, char* name, char* src, char* dst, int mtu);
ssize_t write_tun(struct tun_device* tun, void* data, size_t len);
ssize_t read_tun(struct tun_device* tun, void* data, size_t len);
int poll_read(struct task* tasks, int count);
struct timespec now;
void get_now();
void fatal(char* msg);


#endif
