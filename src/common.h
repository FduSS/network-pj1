#ifndef TRANSFER_COMMON_H
#define TRANSFER_COMMON_H

#define MTU 1500

#include <string.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#include "task.h"

#if defined(LINUX)
#include <fcntl.h>
#include <unistd.h>
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
#include <io.h>
#define A_NAME "tunA"
#define B_NAME "tunB"
const char* inet_ntop(int af, const void* src, char* dst, int cnt);

#else
#error "Unsupported system! This lab only support cygwin/Linux/MacOSX"
#endif

int create_tun(char* name);
void setup_tun(int fd, char* name, char* src, char* dst, int mtu);
ssize_t read_tun(int fd, char* data, size_t len);
ssize_t write_tun(int fd, char* data, size_t len);
struct timespec get_now();
void fatal(char* msg);


#endif
