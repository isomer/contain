#define _GNU_SOURCE 1
#include "contain.h"
#include <sys/resource.h>
#include <sys/time.h>
#include <argp.h>
#include <stdbool.h>
#include <stdlib.h>
#include <err.h>

static struct argp_option nice_options[] = {
    {"nice",       20, "priority", 0, "Set process nice priority", 0},
    {NULL,	   0,   0, 0, NULL, 0 },
};

static int nice_value = 0;
static bool set_nice = false;

static error_t parse_nice_opt(int key, char *arg, struct argp_state *state)
{
    switch(key) {
        case 20:
            nice_value = atoi(arg);
	    set_nice = true;
            break;
	default:
	    return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

struct argp nice_argp = { 
    nice_options, parse_nice_opt, "", "Nice Priority", 0, 0, 0 };

int do_nice(void)
{
    if (set_nice)
        if (setpriority(PRIO_PROCESS, 0, nice_value))
            err(1, "setpriority(PRIO_PROCESS, 0, %d)", nice_value);
    return 0;
}
