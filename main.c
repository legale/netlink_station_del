#include <stdio.h> /* printf() */
#include <stdlib.h> /* exit() */
#include <stdbool.h> /* bool, true, false macros */
#include <unistd.h> /* close() */

#include "station_del.h"


/* cli arguments parse macro and functions */
#define NEXT_ARG() do { argv++; if (--argc <= 0) incomplete_command(); } while(0)
#define NEXT_ARG_OK() (argc - 1 > 0)
#define PREV_ARG() do { argv--; argc++; } while(0)

static char *argv0; /* ptr to the program name string */
static void usage(void) {
    fprintf(stdout, ""
                    "Usage:   %s [command value] ... [command value]    \n"
                    "            command: dev | mac | help              \n"
                    "\n"
                    "Example: %s dev wlan0 mac 00:ff:12:a3:e3:b2        \n"
                    "\n", argv0, argv0);
    exit(-1);
}


/* Returns true if 'prefix' is a not empty prefix of 'string'. */
static bool matches(const char *prefix, const char *string) {
    if (!*prefix)
        return false;
    while (*string && *prefix == *string) {
        prefix++;
        string++;
    }
    return !*prefix;
}

static void incomplete_command(void) {
    fprintf(stderr, "Command line is not complete. Try option \"help\"\n");
    exit(-1);
}

int main(int argc, char **argv) {
    int ret;
    char *dev = NULL, *mac = NULL;
    /* cli arguments parse */
    argv0 = *argv; /* first arg is program name */
    while (argc > 1) {
        NEXT_ARG();
        if (matches(*argv, "dev")) {
            NEXT_ARG();
            dev = *argv; /* device interface name e.g. wlan0 */
        } else if (matches(*argv, "mac")) {
            NEXT_ARG();
            mac = *argv; /* mac address e.g. aa:bb:cc:dd:ee:ff */
        } else if (matches(*argv, "help")) {
            usage();
        } else {
            usage();
        }
    }

    if (dev == NULL || mac == NULL) {
        incomplete_command();
    }
    ret = nl80211_cmd_del_station(dev, mac);
    return -ret;
}

