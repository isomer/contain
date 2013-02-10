#define _GNU_SOURCE
#include "contain.h"
#include <sys/capability.h>
#include <sys/prctl.h>
#include <argp.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct argp_option cap_options[] = {
    {"cap", 'p', "[+|-]cap[,cap...]", 0, "Limit to only these capabilities", 0},
    {NULL, 0, 0, 0, NULL, 0 },
};

static cap_t *containcap = NULL;

static error_t parse_cap_opt(int key, char *arg, struct argp_state *state)
{
    bool enable=true;
    char *st;
    char *end;
    switch(key) {
	case 'p':
	    st = arg;
	    if (*st == '+') {
		enable=true;
		++st;
	    }
	    else if (*st == '-') { 
		enable=false;
		++st;
	    }
	    if (!containcap) {
		/* If we're adding capabilities, start with none.
		 * If we're removing capabilities, start with what we have.
		 */
		containcap = malloc(sizeof(*containcap));
		if (enable)
		    cap_clear(*containcap);
		else {
		    *containcap = cap_get_proc();
		}
	    }
	    /* Parse the remainder of the string. */
	    do {
		char *name;
		end = strchr(st, ',');
		if (end) {
		    name = strndup(st, end-st);
		    st = end;
		}
		else {
		    name = strdup(st);
		    st += strlen(name);
		}
		/* add cap_ onto the beginning of the string if it's not there 
                 */
		if (strncasecmp(name, "cap_", 4) != 0) {
			char *oldname=name;
			asprintf(&name, "cap_%s", oldname);	
			free(oldname);
		}
		cap_value_t cap;
		if (cap_from_name(name, &cap) == -1)
		    errx(1, "Unable to parse capability %s", name);
		if (cap_set_flag(*containcap, CAP_PERMITTED, 1, &cap, enable) 
			== -1)
		    err(1, "Unable to change capability %s in permitted", name);
		if (cap_set_flag(*containcap, CAP_INHERITABLE, 1, &cap, enable) 
			== -1)
		    err(1, "Unable to change capability %s in inheritable", 
				name);
		if (cap_set_flag(*containcap, CAP_PERMITTED, 1, &cap, enable) 
			== -1)
		    err(1, "Unable to change capability %s in permitted", 
				name);
		free(name);
	    } while (*st);
	    break;
	default:
	    return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

struct argp cap_argp = {
    cap_options, parse_cap_opt, "", "Capabilities", 0, 0, 0, };

int do_capabilities(void)
{
    if (containcap) {
	int last_cap=0;
	int i;
	FILE *f = fopen("/proc/sys/kernel/cap_last_cap", "r");
	if (!f)
	    err(1, "Unable to read /proc/sys/kernel/cap_last_cap");
	fscanf(f, "%d", &last_cap);
	fclose(f);

	printf("Proc: %s\n", cap_to_text(cap_get_proc(), NULL));
	printf("Cap: %s\n", cap_to_text(*containcap, NULL));

	if (cap_set_proc(*containcap) == -1)
	    err(1, "Failed to set process capabilities");
	
	for(i=0;i<last_cap+1;++i) {
	    cap_flag_value_t isset;
	    if (cap_get_flag(*containcap, i, CAP_PERMITTED, &isset) == -1)
		err(1, "Unable to discover capability %d", i);
	    /* I suspect I'm going to run into the problem where if I try and
	     * drop SETPCAP that I won't be able to drop other capabilities,
             * so I might have to reorder this
	     */
	    if (isset && prctl(PR_CAPBSET_DROP, i, 0, 0, 0) == -1)
		err(1, "Failed to drop capability %d", i);
	}
	
    }
    return 0;
}
