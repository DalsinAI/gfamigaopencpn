/* ssl/sha1.h (compat/include) on gfopencrypto.library's SHA-1 blocks.
 * Copyright (c) 2026 AmiCradle Team (GPL v2 or later, as OpenCPN) */
#include "ssl/sha1.h"

#include <string.h>

#ifdef __AMIGAOS3__

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/gfopencrypto.h>

struct Library *OpenCryptoBase = NULL;

static int Open(void) {
  static int tried = 0;
  if (!tried) {
    tried = 1;
    OpenCryptoBase = OpenLibrary((CONST_STRPTR) "gfopencrypto.library", 2);
    if (!OpenCryptoBase) OpenCryptoBase = OpenLibrary((CONST_STRPTR) "PROGDIR:Libs/gfopencrypto.library", 2);
    if (OpenCryptoBase && (OC_Version() & 0xffff) < 2) {  /* 2.2 brought SHA-1 */
      CloseLibrary(OpenCryptoBase);
      OpenCryptoBase = NULL;
    }
  }
  return OpenCryptoBase != NULL;
}

static void __attribute__((destructor)) Close(void) {
  if (OpenCryptoBase) CloseLibrary(OpenCryptoBase);
  OpenCryptoBase = NULL;
}

static void Blocks(unsigned char state[20], const unsigned char *data, size_t n) {
  struct OCBlocksRequest r;
  if (!n || !Open()) return;  /* without the library the hash stays unchanged */
  r.state = state;
  r.data = data;
  r.blocks = (ULONG)n;
  OC_SHA1Blocks(&r);
}

#else  /* the host check: GFOpenCrypto's own C code, the code the library runs */

#include <stdint.h>
#include "gfopencrypto/gfopencrypto.h"

static void Blocks(unsigned char state[20], const unsigned char *data, size_t n) {
  if (n) oc_sha1_blocks(state, data, n);
}

#endif

static const unsigned char IV[20] = {0x67, 0x45, 0x23, 0x01, 0xef, 0xcd, 0xab, 0x89, 0x98, 0xba,
                                     0xdc, 0xfe, 0x10, 0x32, 0x54, 0x76, 0xc3, 0xd2, 0xe1, 0xf0};

void sha1_starts(sha1_context *ctx) {
  memcpy(ctx->state, IV, sizeof IV);
  ctx->used = 0;
  ctx->total = 0;
}

void sha1_update(sha1_context *ctx, const unsigned char *in, size_t len) {
  ctx->total += len;
  if (ctx->used) {
    size_t take = 64 - ctx->used;
    if (take > len) take = len;
    memcpy(ctx->block + ctx->used, in, take);
    ctx->used += take;
    in += take;
    len -= take;
    if (ctx->used < 64) return;
    Blocks(ctx->state, ctx->block, 1);
    ctx->used = 0;
  }
  if (len >= 64) {  /* whole blocks straight from the caller's buffer */
    size_t n = len / 64;
    Blocks(ctx->state, in, n);
    in += n * 64;
    len -= n * 64;
  }
  memcpy(ctx->block, in, len);
  ctx->used = len;
}

/* FIPS 180-4 5.1.1: a 1 bit, zeros, and the length in bits, big-endian */
void sha1_finish(sha1_context *ctx, unsigned char out[20]) {
  unsigned long long bits = ctx->total * 8;
  int i;
  ctx->block[ctx->used++] = 0x80;
  if (ctx->used > 56) {
    memset(ctx->block + ctx->used, 0, 64 - ctx->used);
    Blocks(ctx->state, ctx->block, 1);
    ctx->used = 0;
  }
  memset(ctx->block + ctx->used, 0, 56 - ctx->used);
  for (i = 0; i < 8; i++) ctx->block[56 + i] = (unsigned char)(bits >> (56 - 8 * i));
  Blocks(ctx->state, ctx->block, 1);
  memcpy(out, ctx->state, 20);
}

void sha1(const unsigned char *in, size_t len, unsigned char out[20]) {
  sha1_context c;
  sha1_starts(&c);
  sha1_update(&c, in, len);
  sha1_finish(&c, out);
}
