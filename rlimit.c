#define _GNU_SOURCE 1
#include "contain.h"
#include <sys/resource.h>
#include <sys/time.h>
#include <argp.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#define RLIMIT_ID(id, hard) (1030 + (id<<1) + (hard))

#define RLIMIT_OPTION(name, id, unit, comment) \
    {name, RLIMIT_ID(id, 0), unit, 0, comment, 0}, \
    {"max-" name, RLIMIT_ID(id, 1), unit, 0, comment, 0},


static struct argp_option rlimit_options[] = {
    RLIMIT_OPTION("virtual-memory", RLIMIT_AS, "bytes",
		    "Set process maximum virtual memory size")
    RLIMIT_OPTION("core-size", RLIMIT_CORE, "bytes",
		    "Set maximum core dump size")
    RLIMIT_OPTION("cpu-time", RLIMIT_CPU, "seconds",
		    "Set maximum CPU time")
    RLIMIT_OPTION("data-memory", RLIMIT_DATA, "bytes",
		    "Set process maximum data memory size")
    RLIMIT_OPTION("file-size", RLIMIT_FSIZE, "bytes",
		    "Set process maximum writable file size")
    RLIMIT_OPTION("lock-memory", RLIMIT_MEMLOCK, "bytes",
		    "Set process maximum lockable memory")
    RLIMIT_OPTION("message-queue", RLIMIT_MSGQUEUE, "bytes",
		    "Maximum amount of memory consumable for message queues")
    /* TODO: RLIMIT_NICE should go here, but it's semantics are messy */
    RLIMIT_OPTION("file-descriptors", RLIMIT_NOFILE, "count",
		    "Maximum number of file descriptors a process may open")
    RLIMIT_OPTION("processes", RLIMIT_NPROC, "count",
		    "Maximum number of processes that this user may create")
    RLIMIT_OPTION("stack", RLIMIT_STACK, "bytes",
		    "Maximum stacks space")
    {NULL,	   0,   0, 0, NULL, 0 },
};

static struct limit_t {
	int limit_type;
	struct rlimit rlimit;
} *limits = NULL;
static int num_limits = 0;

static void update_limit(int limit_type, const char *limit, bool hard)
{
    int i;
    rlim_t value;
    if (!strcasecmp(limit, "unlimited") || !strcasecmp(limit, "infinity"))
	value = RLIM_INFINITY;
    else {
	char *endptr;
	if (!*limit)
	    errx(1, "Expected a value for limit");
	value = strtoul(limit, &endptr, 0);
	if (*endptr)
	    errx(1, "Invalid character in limit: %c", *endptr);
    }

    for(i=0; i<num_limits; ++i) {
	if (limits[i].limit_type == limit_type) {
		if (hard)
			limits[i].rlimit.rlim_max = value;
		else
			limits[i].rlimit.rlim_cur = value;
		return;
	}
    }

    limits = realloc(limits, sizeof(struct limit_t) * (num_limits+1));
    if (!limits)
	err(1, "Failed to allocate memory for resource limits");

    if (getrlimit(limit_type, &limits[num_limits].rlimit) == -1)
	err(1, "Failed to getrlimit(%d)", limit_type);

    if (hard)
	limits[num_limits].rlimit.rlim_max = value;
    else
	limits[num_limits].rlimit.rlim_cur = value;
    limits[num_limits].limit_type = limit_type;
    ++num_limits;
}

static error_t parse_rlimit_opt(int key, char *arg, struct argp_state *state)
{
    (void) state;
    if (key < 1030 || key >1049)
	    return ARGP_ERR_UNKNOWN;

    /* Update the appropriate limit */
    update_limit((key&0xFF)>>1, arg, key & 1);

    return 0;
}

struct argp rlimit_argp = {
    rlimit_options, parse_rlimit_opt, "", "Nice Priority", 0, 0, 0 };

int do_rlimit(void)
{
    int i;
    for(i=0; i<num_limits; ++i) {
	if (setrlimit(limits[i].limit_type, &limits[i].rlimit))
	    /* It's a pity there isn't a strrlimit(3) */
	    err(1, "Failed to setrlimit(%d)", limits[i].limit_type);
    }
    return 0;
}
