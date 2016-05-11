#include <fcntl.h>
#include <string.h>

#include "../common.h"
#include <mach/mach_time.h>

#define ORWL_NANO (+1.0E-9)
#define ORWL_GIGA UINT64_C(1000000000)

int create_tun(char* name) {
  char path[256];
  strcpy(path, TUN_DEVICE);
  strcat(path, name);

  int fd = open(path, O_RDWR);
  if (fd < 0) {
    fatal("Cannot open tun device");
  }

  int flags = fcntl(fd, F_GETFL, 0);
  int ret = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  if (ret < 0) {
    fatal("fcntl");
  }

  return fd;
}

void setup_tun(int fd, char* name, char* src, char* dst, int mtu) {
  char cmd[128];

  sprintf(cmd, "ifconfig %s %s %s up mtu %d", name, src, dst, mtu);
  system(cmd);
}

void get_now() {
  static double orwl_timebase = 0.0;
  static uint64_t orwl_timestart = 0;

  if (!orwl_timestart) {
    mach_timebase_info_data_t tb = { 0 };
    mach_timebase_info(&tb);
    orwl_timebase = tb.numer;
    orwl_timebase /= tb.denom;
    orwl_timestart = mach_absolute_time();
  }

  double diff = (mach_absolute_time() - orwl_timestart) * orwl_timebase;

  now.tv_sec = (diff * ORWL_NANO);
  now.tv_nsec = (diff - (now.tv_sec * ORWL_GIGA));
}
