CFLAGS?=-g -Wall -Wstrict-prototypes -Wextra -Wmissing-prototypes

PKGS:=libcgroup libselinux

PREFIX?=/usr/local

contain:CFLAGS+=$(shell pkg-config --cflags $(PKGS))
contain:LDFLAGS+=$(shell pkg-config --libs-only-L --libs-only-other $(PKGS))
contain:LDLIBS=$(shell pkg-config --libs-only-l $(PKGS)) -lcap

all: contain simple-init

contain:contain.c caps.c cgroup.c chroot.c clone.c nice.c prctl.c rlimit.c \
	selinux.c ioprio.c idmap.c

install: all
	install -s -t $(DESTDIR)$(PREFIX)/sbin contain simple-init 
	install -t $(DESTDIR)$(PREFIX)/sbin mkcontain

clean:
	rm -f contain simple-init
