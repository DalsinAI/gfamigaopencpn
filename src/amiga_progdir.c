/* The program's own drawer as a full path, for OpenCPN's data and settings
 * folders (patches/0008). Kept in C, apart from wx: dos.h's inline macros
 * (Open, Close, ...) clash with wx's member names.
 * Copyright (c) 2026 AmiCradle Team (GPL v2 or later, as OpenCPN) */
#include <string.h>
#ifdef __amigaos__
#include <proto/dos.h>
#endif

/* "Work:Navigation/OpenCPN/" (or "Work:" at the root of a volume): names
 * can be added to it directly; "PROGDIR:" if the lock has no name */
const char *amiga_program_drawer(void)
{
    static char buf[512];
    if (!buf[0]) {
#ifdef __amigaos__
        BPTR lock = GetProgramDir();
        if (!lock || !NameFromLock(lock, (STRPTR)buf, sizeof buf - 1))
#endif
            strcpy(buf, "PROGDIR:");
        size_t n = strlen(buf);
        if (n && buf[n - 1] != ':' && buf[n - 1] != '/') strcat(buf, "/");
    }
    return buf;
}
