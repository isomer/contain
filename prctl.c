#include "contain.h"
#include <sys/prctl.h>
#include <linux/securebits.h>
#include <argp.h>

static struct argp_option prctl_options[] = {
    {"noroot",      'r',    0, 0, 
	"Setuid root does not grant capabilities", 0},
    {"nonewprivs",   1070,   0, 0,
        "No new privileges that can be granted via execve", 0},
    {NULL,	    0,      0, 0, NULL, 0 },
};

static bool noroot = false;
static bool nonewprivs = false;

static error_t parse_prctl_opt(int key, char *arg, struct argp_state *state)
{
    switch(key) {
        case 'r':
	    noroot = true;
	    break;
        case 1070:
            nonewprivs = true;
            break;
	default:
	    return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

struct argp prctl_argp = { 
    prctl_options, parse_prctl_opt, "", "Process flags", 0, 0, 0 };

int do_prctl(void)
{
    int securebits = 0;
    if (noroot) {
	securebits |= SECBIT_KEEP_CAPS_LOCKED 
                    | SECBIT_NO_SETUID_FIXUP | SECBIT_NO_SETUID_FIXUP_LOCKED
                    | SECBIT_NOROOT | SECBIT_NOROOT_LOCKED;

        if (prctl(PR_SET_SECUREBITS, securebits, 0, 0, 0) == -1)
            return -1;
    }

    if (nonewprivs && prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == -1)
        return -1;

    return 0;
}
