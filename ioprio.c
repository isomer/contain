#define _GNU_SOURCE 1
#include "contain.h"
#include <argp.h>
#include <stdlib.h>
#include <err.h>

#ifndef HAVE_IOPRIO
#include <unistd.h>
#include <sys/syscall.h>

enum {
    IOPRIO_CLASS_NONE,
    IOPRIO_CLASS_RT,
    IOPRIO_CLASS_BE,
    IOPRIO_CLASS_IDLE,
};

enum {
    IOPRIO_WHO_PROCESS = 1,
    IOPRIO_WHO_PGRP,
    IOPRIO_WHO_USER,
};

#define IOPRIO_PRIO_VALUE(class, value) (((class)<<13) | (value))

/* Remove this once libc catches up */
static inline int ioprio_set(int which, int who, int ioprio)
{
        return syscall(__NR_ioprio_set, which, who, ioprio);
}

static inline int ioprio_get(int which, int who)
{
        return syscall(__NR_ioprio_get, which, who);
}
#endif

static struct argp_option ioprio_options[] = {
    {"iort",       1010, "priority", 0, "Set IO Realtime priority level", 0},
    {"iobe",       1011, "priority", 0, "Set IO Besteffort priority level", 0},
    {"ioidle",     1012,  0,         0, "Set IO Idle priority level", 0},
    {NULL,	   0,   0, 0, NULL, 0 },
};

static int ioprio_value = 0;

static error_t parse_ioprio_opt(int key, char *arg, struct argp_state *state)
{
    switch(key) {
        case 1010:
            ioprio_value = IOPRIO_PRIO_VALUE(IOPRIO_CLASS_RT, 0);
            break;
        case 1011:
            ioprio_value = IOPRIO_PRIO_VALUE(IOPRIO_CLASS_BE, atoi(arg));
            break;
        case 1012:
            ioprio_value = IOPRIO_PRIO_VALUE(IOPRIO_CLASS_IDLE, atoi(arg));
            break;
	default:
	    return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

struct argp ioprio_argp = { 
    ioprio_options, parse_ioprio_opt, "", "IO Priority", 0, 0, 0 };

int do_ioprio(void)
{
    if (ioprio_value)
        if (ioprio_set(IOPRIO_WHO_PROCESS, 0, ioprio_value) == -1)
            err(1, "ioprio");
    return 0;
}
