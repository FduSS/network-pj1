//
// Created by phe on 5/3/2016.
//

#ifndef TRANSFER_COMMON_H
#define TRANSFER_COMMON_H

#include <stdio.h>
#include <stdlib.h>

#define MTU 1500

#if defined(LINUX)
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#define TUN_DEVICE "/dev/net/tun"
#define A_NAME "tunA"
#define B_NAME "tunB"

#elif defined(OSX)
#include <netinet/ip.h>
#include <arpa/inet.h>
#define TUN_DEVICE "/dev/"
#define A_NAME "tun8"
#define B_NAME "tun9"

#else
#error "Unsupported system!"
#endif

extern int create_tun(char* name);
extern void setup_tun(char* name, char* src, char* dst, int mtu);
extern void fatal(char* msg);


#endif //TRANSFER_COMMON_H
