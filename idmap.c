#define _GNU_SOURCE 1
#include "contain.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <argp.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* TODO: This file has a lot of duplicated code, refactor mercilessly. */

static struct argp_option id_options[] = {
    {"mapuid", 1080, "newuid[-newuid]=olduid", 0,
        "Map user range, may be provided multiple times", 0},
    {"mapgid", 1081, "newgid[-newgid]=oldgid", 0,
        "Map group range, may be provided multiple times", 0},
    {NULL, 0, NULL, 0, NULL, 0},
};

struct idmap_t {
    int idstart;
    int idend;
    int oldid;
    struct idmap_t *next;
} *uidmap_head=NULL, *uidmap_tail=NULL, *gidmap_head=NULL, *gidmap_tail=NULL;


static struct idmap_t *allocate_id(struct idmap_t **id_head,
        struct idmap_t **id_tail)
{
    struct idmap_t *tmp = malloc(sizeof(struct idmap_t));
    tmp->next = NULL;
    if (*id_tail == NULL)
        *id_head = tmp;
    else
        (*id_tail)->next = tmp;
    (*id_tail) = tmp;
    return tmp;
}


static error_t parse_id_options(int key, char *arg, struct argp_state *state)
{
    struct idmap_t *tmp;
    char *dot, *dot2;
    switch(key) {
        case 1080:
            tmp = allocate_id(&uidmap_head, &uidmap_tail);
            errno = 0;
            tmp->idstart = strtol(arg, &dot, 10);
            if (errno != 0)
                argp_failure(state, 1, 0, "Expected start id in %s map", "uid");
            if (*dot == '-') {
                tmp->idend = strtol(dot+1, &dot2, 10);
                if (errno != 0)
                    argp_failure(state, 1, 0, "Expected end id in %s map",
                            "uid");
                dot = dot2;
            }
            else
                tmp->idend = tmp->idstart;
            if (*dot != '=')
                argp_failure(state, 1, 0, "Expected = in %s map", "uid");
            tmp->oldid = strtol(dot+1, &dot2, 10);
            if (errno != 0)
                argp_failure(state, 1, 0, "Expected old id in %s map", "uid");
            if (*dot2 != '\0')
                argp_failure(state, 1, 0, "Extra cruft after old id in %s map (%s)",
                        "uid", dot2);
            break;
        case 1081:
            tmp = allocate_id(&gidmap_head, &gidmap_tail);
            errno = 0;
            tmp->idstart = strtol(arg, &dot, 10);
            if (errno != 0)
                argp_failure(state, 1, 0, "Expected start id in %s map", "uid");
            if (*dot == '-') {
                tmp->idend = strtol(dot+1, &dot2, 10);
                if (errno != 0)
                    argp_failure(state, 1, 0, "Expected end id in %s map",
                            "uid");
                dot = dot2;
            }
            else
                tmp->idend = tmp->idstart;
            if (*dot != '=')
                argp_failure(state, 1, 0, "Expected = in %s map", "uid");
            tmp->oldid = strtol(dot+1, &dot2, 10);
            if (errno != 0)
                argp_failure(state, 1, 0, "Expected old id in %s map", "uid");
            if (*dot2 != '\0')
                argp_failure(state, 1, 0, "Extra cruft after old id in %s map (%s)",
                        "uid", dot2);
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

struct argp idmap_argp = {
    id_options, parse_id_options, "", "ID Remapping options", 0, 0, 0 };


int do_idmap(int ppid)
{
    char filename[1024];
    struct idmap_t *idmap;
    if (gidmap_head) {
        char *buffer = NULL;
        for (idmap = gidmap_head;
                idmap;
                idmap = idmap->next) {
            char *tmpbuf = NULL;
            asprintf(&tmpbuf, "%s%d %d %d\n",
                    buffer ? buffer : "",
                    idmap->idstart,
                    idmap->oldid,
                    idmap->idend - idmap->idstart + 1);
            if (buffer)
                free(buffer);
            buffer = tmpbuf;
        }
        sprintf(filename, "/proc/%d/gid_map", ppid);
        int fd = open(filename, O_RDWR);
        write(fd, buffer, strlen(buffer));
        close(fd);
        free(buffer);
    }

    if (uidmap_head) {
        char *buffer = NULL;
        for (idmap = uidmap_head;
                idmap;
                idmap = idmap->next) {
            char *tmpbuf = NULL;
            asprintf(&tmpbuf, "%s%d %d %d\n",
                    buffer ? buffer : "",
                    idmap->idstart,
                    idmap->oldid,
                    idmap->idend - idmap->idstart + 1);
            if (buffer)
                free(buffer);
            buffer = tmpbuf;
        }
        sprintf(filename, "/proc/%d/uid_map", ppid);
        int fd = open(filename, O_RDWR);
        write(fd, buffer, strlen(buffer));
        close(fd);
        free(buffer);
    }
    return 0;
}
