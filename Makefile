CFLAGS?=-g -Wall -Wstrict-prototypes -Wextra

PKGS:=libcgroup libselinux

manager:CFLAGS+=$(shell pkg-config --cflags $(PKGS))
manager:LDFLAGS+=$(shell pkg-config --libs-only-L --libs-only-other $(PKGS))
manager:LDLIBS=$(shell pkg-config --libs-only-l $(PKGS)) -lcap

all: manager simple-init

manager:manager.c caps.c cgroup.c chroot.c clone.c prctl.c selinux.c

clean:
	rm -f manager simple-init
