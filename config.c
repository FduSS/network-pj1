#include <sys/time.h>
#include <getopt.h>
#include <stdlib.h>
#include <printf.h>
#include "config.h"

float config_drop_rate;
struct timeval config_delay;

int parse_arg(int argc, char* argv[]) {

  config_drop_rate = 0;
  config_delay.tv_sec = 0;
  config_delay.tv_usec = 0;

  int idx;
  while (1) {
    int opt = getopt(argc, argv, "t:d:");
    if (opt == -1) {
      break;
    }
    switch (opt) {
      case 't':
        printf("Set delay to %s ms\n", optarg);
        config_delay.tv_usec = atoi(optarg);
        break;

      case 'd':
        printf("Set packet drop rate to %s %%\n", optarg);
        config_drop_rate = (float) atoi(optarg) / 100;
        break;

      default:
        return -1;
    }
  }

  return 0;
}
