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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool new_ipc = false;
static bool new_net = false;
static bool new_ns = false;
static bool new_pid = false;
static bool new_uts = false;
static bool new_user = false;
static char **argv = NULL;
static int argc = 0;
static struct passwd *user = NULL;
static struct group *group = NULL;
static char *hostname = NULL;
static bool detach = false;

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
	if (user) {
	    if (initgroups(user->pw_name, user->pw_gid) == -1)
		err(1, "initgroups(%s, %d)", user->pw_name, user->pw_gid);
            if (setgid(user->pw_gid) == -1)
                err(1, "setgid(%d)", user->pw_gid);
        }

    if (user) {
	if (setuid(user->pw_uid) == -1)
	    err(1, "setuid(%d)", user->pw_uid);
    }
}

static int child_start(void *dummy)
{
    (void)dummy;
    if (hostname)
	sethostname(hostname, strlen(hostname));
    if (do_nice() == -1)
        err(1, "nice");
    if (do_ioprio() == -1)
        err(1, "ioprio");
    if (do_cgroup() == -1)
        err(1, "cgroup");
    if (do_chroot() == -1)
        err(1, "chroot");
    if (do_selinux() == -1)
        err(1, "selinux");
    if (do_prctl() == -1)
        err(1, "prctl");
    if (do_capabilities() == -1)
        err(1, "capabilities");
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
    {"newuser", 1052, 0, 0, "Create a new uid/gid namespace", 0},
    {"hostname",1050, "hostname", 0, "Set the hostname", 0},
    {"uid",      'u', "user", 0, "User ID to change to", 0},
    {"gid",      'g', "group", 0, "Group ID to change to", 0},
    {"detach",	1051,  0, 0, "Detach child process", 0},
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
	case 1051:
	    detach = true;
	    break;
	case 1052:
	    new_user = true; break;
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
            if (new_user && !user) {
                user = getpwuid(getuid());
                if (!user)
                    argp_failure(state, 1, 0,
                            "User not specified, and you don't exist");
            }
	    break;
	default:
	    return ARGP_ERR_UNKNOWN;
    }
    return 0;
}


struct argp clone_argp = { clone_options, parse_clone_opt, "", "Clone flags", 0, 0, 0 };

#define UNSHARE_SUPPORTED_FLAGS \
    (CLONE_NEWIPC|CLONE_NEWNET|CLONE_NEWNS|CLONE_NEWUTS|CLONE_FILES \
     |CLONE_NEWUSER)

int setup_clone(void)
{
#define STACKSIZE (32*1024)
    int flags = 0;
    int status;

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
    if (new_user)
	flags |= CLONE_NEWUSER;

    if (detach)
	daemon(true, true);

    /* We have to remap the users independently, because you can't mix
     * NEW_USER with other namespaces safely.
     */
    if ((uidmap_head || gidmap_head) && (flags & CLONE_NEWUSER)) {
        /* We need another process that still has permissions to take care
         * of the rewriting of the uid table.
         */
        int pipes[2]; /* Pipe used for IPC with helper */
        int pid;
        if (pipe(pipes) == -1)
            err(1, "pipe");

        int ppid;
        switch (pid = fork()) {
            case -1:
                err(1, "fork");
            case 0: /* Child */
                /* The child retains it's permissions so it can update the
                 * parents environment.
                 */
                close(pipes[1]);
                /* Wait until the unshare has completed. */
                assert(sizeof(ppid) < PIPE_BUF);
                if (read(pipes[0], &ppid, sizeof(ppid)) != sizeof(ppid))
                    err(1, "unexpected read on pipe");
                close(pipes[0]);
                _exit(do_idmap(ppid));
            default: /* Parent process... */
                close(pipes[0]);
                if (unshare(CLONE_NEWUSER) == -1)
                    err(1, "unshare(CLONE_NEWUSER)");
                /* Signal to the helper that we're ready for it to do its
                 * thing.
                 */
                ppid = getpid();
                if (write(pipes[1], &ppid, sizeof(ppid)) != sizeof(ppid))
                    err(1, "unexpected write on pipe");
                close(pipes[1]);
                /* Wait for the child to have finished creating our
                 * uid/gid mapping. */
                if (waitpid(pid, NULL, 0) == -1)
                    err(1, "waitpid(helper[%d])", pid);

                flags &= ~CLONE_NEWUSER; /* No longer needed. */
        }
    }

    if (flags & ~(UNSHARE_SUPPORTED_FLAGS|SIGCHLD)) {
	char *stack = malloc(STACKSIZE);
	pid_t pid = clone(child_start, stack+STACKSIZE, flags, NULL);
	if (pid == -1)
	    err(1, "clone()");

	if (!detach && waitpid(pid, &status, 0) == -1)
	    warn("waitpid(%d)", pid);
    }
    else {
	if (unshare(flags & ~(SIGCHLD)) == -1)
	    err(1, "unshare");
	child_start(NULL);
    }

    return status;
}

