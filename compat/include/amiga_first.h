/* Included before every C and C++ file of OpenCPN on AmigaOS 3 (-include).
 * - Some OpenCPN headers include <sys/socket.h> before wx's, and libnix's
 *   pulls in exec/lists.h, whose IsListEmpty() macro then breaks wx's
 *   wxComboBox::IsListEmpty(). Taking exec/lists.h first and dropping the
 *   macro keeps the two apart; OpenCPN never uses the exec macro.
 * - exec/types.h defines GLOBAL as extern, which breaks SQLite's GLOBAL(t,v).
 * - libnix has no sigsetjmp()/siglongjmp(); OpenCPN only uses them to step
 *   out of a fault handler, and plain setjmp()/longjmp() do that here.
 * Copyright (c) 2026 AmiCradle Team */
#if defined(__AMIGAOS3__)
#include <exec/types.h>
#include <exec/lists.h>
#undef IsListEmpty
#undef GLOBAL
#include <setjmp.h>
#ifndef sigsetjmp
#define sigjmp_buf jmp_buf
#define sigsetjmp(env, savemask) setjmp(env)
#define siglongjmp(env, val) longjmp(env, val)
#endif
#endif
