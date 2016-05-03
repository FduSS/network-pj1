//
// Created by phe on 5/3/2016.
//

#ifndef TRANSFER_COMMON_H
#define TRANSFER_COMMON_H

#if defined(__linux__)
#define LINUX
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/if_tun.h>
#include <linux/if.h>

#elif defined(_WIN32)
#define WINDOWS

#elif defined(__APPLE__)
#define OSX

#else
#error "Unsupported system!"
#endif


#endif //TRANSFER_COMMON_H
