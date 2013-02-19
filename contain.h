#ifndef MANAGER_H 
#define MANAGER_H 1
#include <stdbool.h>

extern struct argp cap_argp;
extern struct argp cgroup_argp;
extern struct argp chroot_argp;
extern struct argp clone_argp;
extern struct argp ioprio_argp;
extern struct argp nice_argp;
extern struct argp prctl_argp;
extern struct argp selinux_argp;

int do_capabilities(void);
int do_ioprio(void);
int do_nice(void);
int do_prctl(void);
int do_selinux(void);
int setup_clone(void);

#endif
