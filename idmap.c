#define _GNU_SOURCE 1
#include "contain.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <argp.h>
#include <fcntl.h>
#include <grp.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>

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


static int lookup_user(const char *username, size_t len)
{
    struct passwd pw, *ppw;
    size_t buflen = sysconf(_SC_GETPW_R_SIZE_MAX);
    char buf[buflen];
    char *st = strndup(username, len);
    if (getpwnam_r(st, &pw, buf, buflen, &ppw) || !ppw) {
        errno = 0;
        int ret = strtol(st, NULL, 0);
        if (errno)
            ret = -1;
        free(st);
        return ret;
    }
    free(st);
    return ppw->pw_uid;
}


static int lookup_group(const char *groupname, size_t len)
{
    struct group gr, *pgr;
    size_t buflen = sysconf(_SC_GETPW_R_SIZE_MAX);
    char buf[buflen];
    char *st = strndup(groupname, len);
    if (getgrnam_r(groupname, &gr, buf, buflen, &pgr) || !pgr) {
        errno = 0;
        int ret = strtol(st, NULL, 0);
        if (errno)
            ret = -1;
        free(st);
        return ret;
    }
    free(st);
    return pgr->gr_gid;
}


static void add_item(struct argp_state *state, char *arg,
        struct idmap_t **head, struct idmap_t **tail, const char *name,
        int (*lookup_name)(const char *name, size_t len)
        )
{
    struct idmap_t *tmp;
    char *dot, *dot2;

    tmp = allocate_id(head, tail);
    errno = 0;
    dot = strchr(arg, ':');
    dot2 = strchr(arg, '=');

    if (!dot2 || dot2 <= arg)
        argp_failure(state, 1, 0, "Expected = in %s map", name);

    if (dot && dot < dot2) {
        tmp->idstart = lookup_name(arg, dot - arg);
        tmp->idend = lookup_name(dot + 1, dot2 - (dot +1));
    }
    else {
        tmp->idstart = lookup_name(arg, dot2 - arg);
        tmp->idend = tmp->idstart;
    }

    tmp->oldid = lookup_name(dot2 + 1, strlen(dot2) + 1);
}


static error_t parse_id_options(int key, char *arg, struct argp_state *state)
{
    switch(key) {
        case 1080:
            add_item(state, arg, &uidmap_head, &uidmap_tail, "uid",
                    lookup_user);
            break;
        case 1081:
            add_item(state, arg, &gidmap_head, &gidmap_tail, "gid",
                    lookup_group);
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
    /* TODO: Sort, and break apart overlapped ranges, and merge consequitive
     * ranges etc.
     */
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
