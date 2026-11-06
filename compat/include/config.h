/* OpenCPN's config.h for AmigaOS 3 (CMake makes it from
 * cmake/in-files/config.h.in on other systems).
 * Copyright (c) 2026 AmiCradle Team */
#ifndef OCPN_CONFIG_H
#define OCPN_CONFIG_H

#define HAVE_OCPN_LOG

/* the new serial code, on our serial.device driver (src/serial_amiga.cc) */
#define OCPN_USE_NEWSERIAL

#define HAVE_DIRENT_H
#define HAVE_UNISTD_H
#define HAVE_SYS_STAT_H
#define HAVE_SYS_TYPES_H

/* plugins are shared libraries elsewhere: there are none on AmigaOS 3 */
#define PKG_TARGET "amigaos3-m68k"
#define PKG_TARGET_VERSION "3.2"

#define CMAKE_BINARY_DIR "PROGDIR:"
#define TEST_ROOT "PROGDIR:test"
#define COMPILER_SUPPORTS_CXX11

#define VERSION_MAJOR 5
#define VERSION_MINOR 14
#define VERSION_PATCH 2
#define VERSION_DATE "2026-09-24"
#define VERSION_FULL "5.14.2"
#define PACKAGE_VERSION "5.14.2"
#define DEBIAN_PPA_VERSION ""

#endif
