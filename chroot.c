#include "manager.h"
#include <sys/mount.h>
#include <argp.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static struct argp_option chroot_options[] = {
    {"bind",	'b', "newdir=olddir", 0, "Bind a directory into the chroot", 0},
    {"unmount",  1,  "directory", 0, "Unmount a directory", 0},
    {"move",    'm', "newdir=olddir", 0, "Move a mount point", 0},
    {"chroot",  'c', "dir", 0, "Chroot to this directory", 0},
    {NULL, 0, NULL, 0, NULL, 0},
};

struct mounts_t {
    enum { MOUNT_BIND, MOUNT_UNMOUNT, MOUNT_MOVE } type;
    const char *source;
    const char *target;
    unsigned long flags;
    const char *data;
    struct mounts_t *next;
} *mounts_head=NULL, *mounts_tail=NULL;

char *root = NULL;

struct mounts_t *allocate_mount(void)
{
	struct mounts_t *tmp = malloc(sizeof(struct mounts_t));
	tmp->next = NULL;
	if (mounts_tail == NULL)
	    mounts_head = tmp;
	else
	    mounts_tail->next = tmp;
	mounts_tail = tmp;
	return tmp;
}

static error_t parse_chroot_opt(int key, char *arg, struct argp_state *state)
{
    struct mounts_t *tmp;
    char *dot;
    /* TODO: Support target=source,option,option,option. */
    switch(key) {    
	case 'b':
	    /* Parse line */
	    dot = strchr(arg, '=');
	    if (!dot || dot <= arg)
		argp_failure(state, 1, 0, "Expected = in bind mount spec");
	    tmp = allocate_mount();
	    tmp->target = strdup(dot+1);
	    tmp->source = strndup(arg, dot-arg-1);
	    tmp->type = MOUNT_BIND;
	    break;
	case 1:
	    tmp = allocate_mount();
	    tmp->target = strdup(arg);
	    tmp->type = MOUNT_UNMOUNT;
	    break;
	case 'm':
	    dot = strchr(arg, '=');
	    if (!dot || dot <= arg)
		argp_failure(state, 1, 0, "Expected = in bind mount spec");
	    tmp = allocate_mount();
	    tmp->target = strdup(dot+1);
	    tmp->source = strndup(arg, dot-arg-1);
	    tmp->type = MOUNT_MOVE;
	    break;
	case 'c':
	    if (root)
		argp_failure(state, 1, 0, 
			    "You cannot specify chroot multiple times");
	    root = strdup(arg);
	    break;
	default:
	    return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

struct argp chroot_argp = {
    chroot_options, parse_chroot_opt, "", "Chroot flags", 0, 0, 0 };

int do_chroot(void)
{
    struct mounts_t *it = NULL;
    /* TODO: Implement. */
    /* TODO: Should I try and umount everything outside the chroot? */
    for(it = mounts_head; it != NULL; it=it->next) {
	switch(it->type) {
	    case MOUNT_UNMOUNT:
		if (umount2(it->target, MNT_DETACH) == -1)
		    err(1, "Unable to unmount(%s)", it->target);
		break;
	    case MOUNT_BIND:
		if (mount(it->source, it->target, NULL, MS_BIND, NULL) == -1)
		    err(1, "Unable to bind mount %s=%s", it->target, 
			it->source);
		break;
	    case MOUNT_MOVE:
		if (mount(it->source, it->target, NULL, MS_MOVE, NULL) == -1)
		    err(1, "Unable to bind mount %s=%s", it->target, 
			it->source);
		break;
	}
    }
    if (root) {
	chroot(root);
	chdir("/"); /* Should we chdir to $HOME? If so, how? */
    }
    return 0;
}
