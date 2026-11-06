/* OpenCPN's SHA-1 calls (s57chart.cpp's chart names, flex_hash.cpp), on
 * gfopencrypto.library's OC_SHA1Blocks (GFOpenCrypto 2.2, version 2): in
 * place of libs/ssl_sha1, which this port does not build. The library is
 * shipped in the program's Libs/ and opened on first use (LIBS:, then
 * PROGDIR:Libs/); the host check build calls GFOpenCrypto's C code.
 * src/sha1_gfcrypto.c holds the code.
 * Copyright (c) 2026 AmiCradle Team (GPL v2 or later, as OpenCPN) */
#ifndef OCPN_SHA1_GFCRYPTO_H
#define OCPN_SHA1_GFCRYPTO_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  unsigned char state[20]; /* the chaining value, big-endian, as the library keeps it */
  unsigned char block[64];
  size_t used;
  unsigned long long total;
} sha1_context;

void sha1_starts(sha1_context *ctx);
void sha1_update(sha1_context *ctx, const unsigned char *input, size_t ilen);
void sha1_finish(sha1_context *ctx, unsigned char output[20]);
void sha1(const unsigned char *input, size_t ilen, unsigned char output[20]);

#ifdef __cplusplus
}
#endif

#endif
