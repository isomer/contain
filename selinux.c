#include "contain.h"
#include <selinux/selinux.h>
#include <argp.h>
#include <err.h>
#include <errno.h>

static struct argp_option selinux_options[] = {
    {"context",	'c', "context", 0, "Set SELinux context", 0},
    {NULL, 		0, 0, 0, NULL, 0 },
};

static security_context_t context;
static bool set_context = false;

static error_t parse_selinux_opt(int key, char *arg, struct argp_state *state)
{
    switch(key) {
	case 'c':
	    if (security_get_initial_context(arg, &context) == -1)
		argp_failure(state, 1, errno, 
		    "Failed to create selinux context");
	    set_context = true;
	    break;
	default:
	    return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

struct argp selinux_argp = {
    selinux_options, parse_selinux_opt, "", "SELinux flags", 0, 0, 0 };

int do_selinux(void)
{
    if (set_context)
	if (setexeccon(context) == -1)
	    err(1, "Unable to set the SELinux exec context");
    return 0;
}
