#define _GNU_SOURCE 1
#include "contain.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <argp.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static struct argp_option id_options[] = {
    {"mapuid", 1080, "newuid[:endnewuid]=olduid", 0,
        "Map user range, may be provided multiple times", 0},
    {"mapgid", 1081, "newgid[:endnewgid]=oldgid", 0,
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


static void add_item(struct argp_state *state, char *arg,
        struct idmap_t **head, struct idmap_t **tail, char *name)
{
    struct idmap_t *tmp;
    char *dot, *dot2;
    tmp = allocate_id(head, tail);
    errno = 0;
    tmp->idstart = strtol(arg, &dot, 10);
    if (errno != 0)
        argp_failure(state, 1, 0, "Expected start id in %s map", name);
    if (*dot == '-') {
        tmp->idend = strtol(dot+1, &dot2, 10);
        if (errno != 0)
            argp_failure(state, 1, 0, "Expected end id in %s map", name);
        dot = dot2;
    }
    else
        tmp->idend = tmp->idstart;
    if (*dot != '=')
        argp_failure(state, 1, 0, "Expected = in %s map", name);
    tmp->oldid = strtol(dot+1, &dot2, 10);
    if (errno != 0)
        argp_failure(state, 1, 0, "Expected old id in %s map", name);
    if (*dot2 != '\0')
        argp_failure(state, 1, 0, "Extra cruft after old id in %s map (%s)",
                name, dot2);
}


static error_t parse_id_options(int key, char *arg, struct argp_state *state)
{
    switch(key) {
        case 1080:
            add_item(state, arg, &uidmap_head, &uidmap_tail, "uid");
            break;
        case 1081:
            add_item(state, arg, &gidmap_head, &gidmap_tail, "gid");
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

struct argp idmap_argp = {
    id_options, parse_id_options, "", "ID Remapping options", 0, 0, 0 };


static void write_map(struct idmap_t *head, int ppid, const char *map)
{
    char filename[1024];
    char *buffer = NULL;
    struct idmap_t *idmap;
    for (idmap = head;
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
    sprintf(filename, "/proc/%d/%s_map", ppid, map);
    int fd = open(filename, O_RDWR);
    write(fd, buffer, strlen(buffer));
    close(fd);
    free(buffer);
}

int do_idmap(int ppid)
{
    if (gidmap_head) {
        write_map(gidmap_head, ppid, "gid");
    }

    if (uidmap_head) {
        write_map(uidmap_head, ppid, "uid");
    }
    return 0;
}
