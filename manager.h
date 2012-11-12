#ifndef MANAGER_H 
#define MANAGER_H 1
#include <stdbool.h>

extern struct argp clone_argp;
extern struct argp prctl_argp;
extern struct argp cgroup_argp;
extern struct argp chroot_argp;
extern struct argp selinux_argp;
extern struct argp cap_argp;

int do_prctl(void);
int setup_clone(void);
int do_selinux(void);
int do_capabilities(void);

#endif
