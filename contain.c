#define _GNU_SOURCE 1
#include "contain.h"
#include "cgroup.h"
#include <argp.h>
#include <err.h>
#include <libcgroup.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

const char *argp_program_version = "contain 1.0";

static char doc[] =
    "Program to flexibly create a new container and run a process";

static struct argp_option options[] = {
    {"verbose", 'v',    0,	 0, "Produce verbose output", 0 },
    {"name",    'n',	"NAME",  0, "Name of the container", 0 },
    {NULL,       0,     0,	 0, NULL, 0 }};

static bool verbose = false;
const char *name = NULL;

static error_t
parse_opt(int key, char *arg, struct argp_state *state)
{
    switch(key) {
	case 'v':
	    verbose = true;
	    break;
	case 'n':
	    if (name) {
		argp_usage(state);
	    }
	    name = strdup(arg);
	    break;
	default:
	    return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp_child argp_children[] = {
    { &cap_argp, 0, "Capabilities", 1 },
    { &cgroup_argp, 0, "Limits", 1 },
    { &chroot_argp, 0, "Filesystem", 1 },
    { &clone_argp, 0, "Clone options", 1 },
    { &ioprio_argp, 0, "IO Prioritisation", 1 },
    { &nice_argp, 0, "Nice prioritisation", 1 },
    { &prctl_argp, 0, "Security options", 1 },
    { &rlimit_argp, 0, "Ulimit options", 1 },
    { &selinux_argp, 0, "SELinux", 1 },
    { &idmap_argp, 0, "ID mapping options", 1 },
    { NULL, 0, NULL, 0 },
};

static struct argp argp = { options, parse_opt, "commandline", doc,
				argp_children, 0, 0 };


int main(int argc, char *argv[])
{
    cgroup_init();
    argp_parse(&argp, argc, argv, 0, 0, NULL);

    return setup_clone();

}
