#define _GNU_SOURCE 1
#include "contain.h"
#include <sys/mount.h>
#include <argp.h>
#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static struct argp_option chroot_options[] = {
    {"bind",	'b', "newdir=olddir", 0, "Bind a directory into the chroot", 0},
    {"unmount",  1,  "directory", 0, "Unmount a directory", 0},
    {"move",    'm', "newdir=olddir", 0, "Move a mount point", 0},
    {"mount",   'M', "target=source,fstype[,flags][,data]", 0, 
	    "Mount a directory", 0 },
    {"chroot",  'c', "dir", 0, "Chroot to this directory", 0},
    {NULL, 0, NULL, 0, NULL, 0},
};

struct mounts_t {
    enum { MOUNT_BIND, MOUNT_UNMOUNT, MOUNT_MOVE, MOUNT_MOUNT } type;
    const char *source;
    const char *target;
    const char *fstype;
    unsigned long flags;
    const char *data;
    struct mounts_t *next;
} *mounts_head=NULL, *mounts_tail=NULL;

char *root = NULL;

static struct mounts_t *allocate_mount(void)
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
    char *dot,*dot2,*dot3;
    switch(key) {    
	case 'b':
	    /* Parse line */
	    dot = strchr(arg, '=');
	    if (!dot || dot <= arg)
		argp_failure(state, 1, 0, "Expected = in bind mount spec");
	    tmp = allocate_mount();
	    tmp->source = strdup(dot+1);
	    tmp->target = strndup(arg, dot-arg);
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
		argp_failure(state, 1, 0, "Expected = in move mount spec");
	    tmp = allocate_mount();
	    tmp->source = strdup(dot+1);
	    tmp->target = strndup(arg, dot-arg);
	    tmp->type = MOUNT_MOVE;
	    break;
	case 'M':
	    dot = strchr(arg, '=');
	    if (!dot || dot <= arg)
		argp_failure(state, 1, 0, "Expected = in mount spec");
	    tmp = allocate_mount();
	    tmp->type = MOUNT_MOUNT;
	    tmp->target = strndup(arg, dot-arg);
	    dot2 = strchr(dot+1, ',');
	    if (!dot2 || dot2 <= dot)
		argp_failure(state, 1, 0, "Expected ,fstype in mount spec");
	    tmp->source = strndup(dot+1, dot2-dot-1);
	    tmp->flags = 0;
	    dot3 = strchr(dot2+1, ',');
	    if (dot3 && dot3 > dot2) {
	    	tmp->fstype = strndup(dot2+1, dot3-dot2-1);
		while (*dot3) {
			while (*dot3 == ',') dot3++;
#define flag(name) \
	if (!strncasecmp(dot3, #name ",", strlen(#name ",")) \
			|| !strcasecmp(dot3, #name)) { \
		dot3 += strlen(#name); \
		tmp->flags |= MS_ ## name; \
		continue; \
	}
			flag(DIRSYNC); flag(MANDLOCK); flag(NOATIME);
			flag(NODEV); flag(NODIRATIME); flag(NOEXEC);
			flag(NOSUID); flag(RDONLY); flag(RELATIME);
			flag(SILENT); flag(STRICTATIME); flag(REMOUNT);
			flag(SYNCHRONOUS);
#undef flag

			// Everything else ends up in "data"
			tmp->data = strdup(dot3);
			break;
		}
	    }
	    else {
		tmp->fstype = strdup(dot2+1);
		tmp->data = strdup("");
	    }
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

static char *target2root(const char *path)
{
	char *ret;
	if (!root)
		return strdup(path);
	asprintf(&ret, "%s/%s", root, path);
	return ret;
}

int do_chroot(void)
{
    struct mounts_t *it = NULL;
    /* TODO: Should I try and umount everything outside the chroot? */
    for(it = mounts_head; it != NULL; it=it->next) {
	switch(it->type) {
	    case MOUNT_UNMOUNT:
		if (umount2(target2root(it->target), MNT_DETACH) == -1)
		    err(1, "Unable to unmount(%s)", it->target);
		break;
	    case MOUNT_BIND:
		if (mount(it->source, target2root(it->target), NULL, MS_BIND, 
					NULL) == -1)
		    err(1, "Unable to bind mount %s=%s", it->target, 
			it->source);
		break;
	    case MOUNT_MOVE:
		if (mount(it->source, target2root(it->target), NULL, MS_MOVE, 
					NULL) == -1)
		    err(1, "Unable to bind mount %s=%s", it->target, 
			it->source);
		break;
	    case MOUNT_MOUNT:
		if (mount(it->source, target2root(it->target), it->fstype, 
					it->flags, it->data) == -1)
	  	    err(1, "Unable to mount %s=%s (%s%s)", it->target,
			it->source, it->fstype, it->data);
		break;
	     default:
		err(1, "unknown type %d", it->type);
	}
    }
    if (root) {
	chroot(root);
	chdir("/"); /* Should we chdir to $HOME? If so, how? */
    }
    return 0;
}
