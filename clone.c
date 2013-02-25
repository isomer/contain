#define _GNU_SOURCE 1
#include "contain.h"
#include "cgroup.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <argp.h>
#include <err.h>
#include <grp.h>
#include <pwd.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool new_ipc = false;
static bool new_net = false;
static bool new_ns = false;
static bool new_pid = false;
static bool new_uts = false;
static char **argv = NULL;
static int argc = 0;
static struct passwd *user = NULL;
static struct group *group = NULL;
static char *hostname = NULL;

/* If:
 * !user && !group => nothing
 *  user && !group => change to user, initgroups with gid
 *  user &&  group => change to user, change to gid (no initgroups)
 * !user &&  group => invalid
 */
static void change_user(void)
{
    if (group) {
	if (setgid(group->gr_gid) == -1) 
	    err(1, "setgid(%d)", group->gr_gid);
    } else
	if (user)
	    if (initgroups(user->pw_name, user->pw_gid) == -1)
		err(1, "initgroups(%s, %d)", user->pw_name, user->pw_gid);

    if (user) {
	if (setuid(user->pw_uid) == -1)
	    err(1, "setuid(%d)", user->pw_uid);
    }
}

static int child_start(void *dummy)
{
    if (hostname)
	sethostname(hostname, strlen(hostname));
    do_nice();
    do_ioprio();
    do_cgroup();
    do_chroot();
    do_selinux();
    do_prctl();
    do_capabilities();
    change_user();
    execvp(argv[0], argv);
    err(1, "execlv(%s)", argv[0]);
}


static struct argp_option clone_options[] = {
    {"newipc",   'I', 0, 0, "Create a new IPC namespace", 0},
    {"newnet",   'N', 0, 0, "Create a new networking namespace", 0},
    {"newmount", 'M', 0, 0, "Create a new filesystem namespace", 0},
    {"newpid",   'P', 0, 0, "Create a new pid namespace", 0},
    {"newuname", 'U', 0, 0, "Create a new uname namespace", 0},
    {"hostname",1050, "hostname", 0, "Set the hostname", 0},
    {"uid",      'u', "user", 0, "User ID to change to", 0},
    {"gid",      'g', "group", 0, "Group ID to change to", 0},
    {NULL,	  0,   0, 0, NULL, 0 },
};

static error_t
parse_clone_opt(int key, char *arg, struct argp_state *state)
{
    switch(key) {
	case 'I': new_ipc = true; break;
	case 'N': new_net = true; break;
	case 'M': new_ns = true; break;
	case 'P': new_pid = true; break;
	case 'U': new_uts = true; break;
	case 'u': 
	    user = getpwnam(arg);
	    if (!user)
		argp_failure(state, 1, errno, "Unknown user %s", arg);
	    break;
	case 'g':
	    group = getgrnam(arg);
	    if (!group)
		argp_failure(state, 1, errno, "Unknown group %s", arg);
	    break;
	case 1050:
	    if (hostname)
		argp_failure(state, 1, errno, "Cannot specify hostname twice");
	    else
	        hostname = strdup(arg);
	    break;
	case ARGP_KEY_ARGS:
	    argv = state->argv + state->next;
	    argc = state->argc - state->next;
	    break;
	case ARGP_KEY_END:
	    if(!argv)
		argp_usage(state);
	    if(group && !user)
		argp_failure(state, 1, 0, 
			    "If you specify a group, you must specify a user");
	    if (hostname && !new_uts)
		argp_failure(state, 1, 0,
			"You must create a new uts namespace with hostname");
	    break;
	default: 
	    return ARGP_ERR_UNKNOWN;
    }
    return 0;
}


struct argp clone_argp = { clone_options, parse_clone_opt, "", "Clone flags", 0, 0, 0 };


int setup_clone(void)
{
#define STACKSIZE (32*1024)
    char *stack = malloc(STACKSIZE);
    int flags = 0;
    int status;
    pid_t pid;

    flags |= CLONE_FILES; /* fork() normally shares fd's */
    flags |= SIGCHLD;	/* Signal to send to the parent process. */

    if (new_ipc)
	flags |= CLONE_NEWIPC;
    if (new_net)
	flags |= CLONE_NEWNET;
    if (new_ns)
	flags |= CLONE_NEWNS;
    if (new_pid)
	flags |= CLONE_NEWPID;
    if (new_uts)
	flags |= CLONE_NEWUTS;
    /* UID/GID namespaces are coming.... */

    pid = clone(child_start, stack+STACKSIZE, flags, NULL);
    if (pid == -1)
	err(1, "clone()");

    if (waitpid(pid, &status, 0) == -1)
	warn("waitpid(%d)", pid);

    return status;
}

