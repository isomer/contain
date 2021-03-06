#ifndef MANAGER_H 
#define MANAGER_H 1
#include <stdbool.h>

extern struct idmap_t *uidmap_head, *uidmap_tail, *gidmap_head, *gidmap_tail;

extern struct argp cap_argp;
extern struct argp cgroup_argp;
extern struct argp chroot_argp; /* 106x */
extern struct argp clone_argp;  /* 105x */
extern struct argp ioprio_argp; /* 101x */
extern struct argp nice_argp;   /* 102x */
extern struct argp prctl_argp;  /* 107x */
extern struct argp rlimit_argp; /* 103x 104x */
extern struct argp selinux_argp;
extern struct argp idmap_argp; /* 108x */

int do_capabilities(void);
int do_chroot(void);
int do_ioprio(void);
int do_nice(void);
int do_prctl(void);
int do_rlimit(void);
int do_selinux(void);
int do_idmap(int ppid);
int setup_clone(void);

#endif
