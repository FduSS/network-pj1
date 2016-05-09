#ifndef TRANSFER_COMMON_H
#define TRANSFER_COMMON_H

#define MTU 1500

#include <string.h>
#include <math.h>

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

#elif defined(CYGWIN)
#include <w32api/winsock2.h>
#include <stdio.h>
#define A_NAME "A"
#define B_NAME "B"

#else
#error "Unsupported system! This lab only support cygwin/Linux/MacOSX"
#endif

extern int create_tun(char* name);
extern void setup_tun(char* name, char* src, char* dst, int mtu);
extern void get_now(struct timespec* t);
extern void fatal(char* msg);


#endif
