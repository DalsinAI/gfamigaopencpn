/* C library calls OpenCPN uses that libnix lacks.
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

/* fchmod and fchown, which SQLite calls, come with gfopensqlite's compat. */
