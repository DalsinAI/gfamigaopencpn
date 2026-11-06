/* C library calls OpenCPN and its SQLite use that libnix lacks.
 * Copyright (c) 2026 AmiCradle Team (GPL v2 or later, as OpenCPN) */
#include <string.h>
#include <sys/types.h>
#include <time.h>

/* ctime() into the caller's buffer (at least 26 bytes, as POSIX says). */
char *ctime_r(const time_t *t, char *buf)
{
    char *s = ctime(t);
    if (!s || !buf) return NULL;
    strcpy(buf, s);
    return buf;
}

/* The Amiga file system has no Unix modes or owners to set on an open file;
 * SQLite only uses these to copy a database's mode to its journal. */
int fchmod(int fd, mode_t mode) { (void)fd; (void)mode; return 0; }
int fchown(int fd, uid_t owner, gid_t group) { (void)fd; (void)owner; (void)group; return 0; }
