#define _GNU_SOURCE 1
#include "cgroup.h"
#include <argp.h>
#include <libcgroup.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

static int num_settings = 0;
static char **setting = NULL;
char *name;

static struct argp_option cgroup_options[] = {
    {"limit",	'l', "<controller>.<resource>=<value>", 0, "Add a limit", 0},
    {NULL,	0, 0, 0, NULL, 0 },
};

static error_t
parse_cgroup_opt(int key, char *arg, struct argp_state *state)
{
    switch(key) {
	case 'l':
	    setting = realloc(setting, (num_settings+2)*sizeof(char *));
	    if (!setting)
		argp_failure(state, 1, errno, "realloc");
	    setting[num_settings]=strdup(arg);
	    setting[num_settings+1]=NULL;
	    num_settings++;
	    break;
	default:
	    return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

struct argp cgroup_argp = { 
    cgroup_options, 
    parse_cgroup_opt, 
    "",
    "Resource limits",
    0,
    0,
    0
};
		
static void cgroup_err(int err, const char *fmt, ...)
{
    va_list va;
    char *buf;

    va_start(va, fmt);
    vasprintf(&buf, fmt, va);
    va_end(va);

    errx(1, "%s: %s", buf, cgroup_strerror(err));
    free(buf); /* Not reached */
}

int do_cgroup(void)
{
    struct cgroup_controller *controller;
    struct cgroup *group;
    int i;
    int err;

    if (!setting)
	return 0; /* Nothing to do */

    if (!name)
	errx(1, "You must specify a name for your cgroup");

    group = cgroup_new_cgroup(name);

    if (!group)
	errx(1, "Failed to create cgroup %s", name);

    for(i=0; i<num_settings; ++i) {
	char *dot = strchr(setting[i], '.');
	if (!dot)
	    errx(1, "Unable to parse %s", setting[i]);

	char *equals = strchr(dot, '=');
	if (!equals)
	    errx(1, "Unable to parse %s", setting[i]);

	const char *controller_name = strndup(setting[i], dot-setting[i]);
	controller = cgroup_get_controller(group, controller_name);
	
	if (!controller)
	    controller = cgroup_add_controller(group, controller_name);

	if (!controller)
	    errx(1, "Unable to create controller for %s", controller_name);

	char *name = strndup(setting[i], equals-(setting[i]));

	err = cgroup_add_value_string(controller, name, equals+1);
	free(name);

	if (err)
	    cgroup_err(err, "Failed to add value for %s", setting[i]);
    }

    if ((err=cgroup_create_cgroup(group, 1)))
	cgroup_err(err, "Failed to create group");

    if ((err=cgroup_attach_task(group)))
	cgroup_err(err, "Failed to attach to group");


    return 0;
}

