CFLAGS?=-g -Wall -Wstrict-prototypes -Wextra

PKGS:=libcgroup libselinux

contain:CFLAGS+=$(shell pkg-config --cflags $(PKGS))
contain:LDFLAGS+=$(shell pkg-config --libs-only-L --libs-only-other $(PKGS))
contain:LDLIBS=$(shell pkg-config --libs-only-l $(PKGS)) -lcap

all: contain simple-init

contain:contain.c caps.c cgroup.c chroot.c clone.c prctl.c selinux.c ioprio.c

clean:
	rm -f contain simple-init
