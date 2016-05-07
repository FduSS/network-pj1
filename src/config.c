#include <sys/time.h>
#include <getopt.h>
#include <stdlib.h>
#include <printf.h>
#include "config.h"

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
    int opt = getopt(argc, argv, "t:d:r:l:");
    if (opt == -1) {
      break;
    }
    switch (opt) {
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
