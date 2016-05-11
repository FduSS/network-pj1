#include <stdlib.h>
#include <stdio.h>
#include "config.h"

#ifdef _MSC_VER
#include "./platform/win_getopt.h"
#else

#include <getopt.h>

#endif

double config_drop_rate, config_delay_trashing;
int config_delay;
long long config_speed_limit;

int parse_arg(int argc, char* argv[]) {

  config_drop_rate = 0;
  config_delay = 0;
  config_delay_trashing = 0;
  config_speed_limit = 0;

  int idx;
  while (1) {
    int opt = getopt(argc, argv, "t:d:r:l:h");
    if (opt == -1) {
      break;
    }
    switch (opt) {
      case 'h':
        printf("Usage:\n");
        printf("\t-t\tSet delay (ms)\n");
        printf("\t-r\tSet delay trashing rate (%%)\n");
        printf("\t-d\tSet pack drop rate (%%)\n");
        printf("\t-l\tSet speed limit (Mbps)\n");
        exit(0);
        break;

      case 't':
        printf("Set delay to %s ms\n", optarg);
        config_delay = atoi(optarg);
        break;

      case 'd':
        printf("Set packet drop rate to %s %%\n", optarg);
        config_drop_rate = (double) atoi(optarg) / 100;
        break;

      case 'r':
        printf("Set delay trashing to %s %%\n", optarg);
        config_delay_trashing = (double) atoi(optarg) / 100;
        break;

      case 'l':
        printf("Set speed limit to %s Mbps\n", optarg);
        config_speed_limit = atoi(optarg) * 1024LL * 1024 / 8;
        break;

      default:
        return -1;
    }
  }

  return 0;
}
