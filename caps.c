#define _GNU_SOURCE
#include "contain.h"
#include <sys/capability.h>
#include <sys/prctl.h>
#include <assert.h>
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

static int set_cap(const char *name, bool enable) 
{
    cap_value_t cap;
    if (cap_from_name(name, &cap) == -1)
	    errx(1, "Unable to parse capability %s", name);

    if (cap_set_flag(*containcap, CAP_INHERITABLE, 1, &cap, enable) 
		    == -1)
	    err(1, "Unable to change capability %s in inheritable", 
			    name);

    return 0;
}

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
		*containcap = cap_get_proc();
		if (enable) {
		    if (cap_clear_flag(*containcap, CAP_INHERITABLE)) 
			    err(1, "cap_clear");
		}
	    }
	    assert(containcap);
	    /* Parse the remainder of the string. */
	    do {
		char *name;
		end = strchr(st, ',');
		if (end) {
		    name = strndup(st, end-st);
		    st = end+1;
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
		set_cap(name, enable);
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

static void drop_cap(int capid)
{
    cap_flag_value_t isset;
    if (cap_valid(capid)) {
	if (cap_get_flag(*containcap, capid, CAP_INHERITABLE, &isset) == -1) {
		/* This is usually due to libcap2 being out of date */
	    	/*err(1, "Unable to discover capability %d", capid);*/
		isset = true; /* Just drop the capability */
	}
    } else {
	/* Dunno what this is, but we probably don't want it. */
	isset = true;	
    }
    if (!isset && prctl(PR_CAPBSET_DROP, capid, 0, 0, 0) == -1)
	err(1, "Failed to drop capability %s", cap_to_name(capid));
}

int do_capabilities(void)
{
    if (containcap) {
	int last_cap=0;
	int i;

	/* Find out what capabilities this kernel supports */
	FILE *f = fopen("/proc/sys/kernel/cap_last_cap", "r");
	if (!f)
	    err(1, "Unable to read /proc/sys/kernel/cap_last_cap");
	fscanf(f, "%d", &last_cap);
	fclose(f);

	for(i=0;i<last_cap+1;++i) {
		/* Drop SETPCAP last */
		if (i != CAP_SETPCAP) drop_cap(i);
	}

	/* This needs to be dropped last, otherwise I can't drop later
	 * capabilities */
	drop_cap(CAP_SETPCAP);

	if (cap_set_proc(*containcap) == -1)
	    err(1, "Failed to set process capabilities");

	
    }
    return 0;
}
