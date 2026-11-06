/* ocpn_json.h on json.library (Amiga) or on its core (host check).
 * Copyright (c) 2026 AmiCradle Team (GPL v2 or later, as OpenCPN) */
#include "ocpn_json.h"

#include <string.h>

/* A NUMBER's text, read from the block as json.library's header documents
 * it (libraries/json.h: JSON_NODES, JSON_NTEXT): the same layout on the
 * Amiga and in the core */
namespace {
struct JsonBlock {
  unsigned int magic, size, nodes, node_off, pool_off, pool_size, flags, reserved;
};
struct JsonNode {
  unsigned int head, data;
};
}  // namespace
const char *ojl::NumberText(const Doc *doc, unsigned node, size_t *len) {
  const JsonBlock *b = (const JsonBlock *)doc;
  if (!b || node >= b->nodes) return 0;
  const JsonNode *n = (const JsonNode *)((const char *)b + b->node_off) + node;
  if ((n->head >> 28) != ojl::NUMBER) return 0;
  if (len) *len = n->data;
  return (const char *)b + b->pool_off + (n->head & 0x0FFFFFFFu);
}

#ifdef __AMIGAOS3__

#include <exec/types.h>
#include <libraries/json.h>
#include <proto/exec.h>
#include <proto/json.h>

struct Library *JSONBase = NULL;

namespace {
struct Opener {
  bool tried = false;
  ~Opener() {
    if (JSONBase) CloseLibrary(JSONBase);
    JSONBase = NULL;
  }
  bool Open() {
    if (!tried) {
      tried = true;
      JSONBase = OpenLibrary((CONST_STRPTR) "json.library", 1);
      if (!JSONBase)
        JSONBase = OpenLibrary((CONST_STRPTR) "PROGDIR:Libs/json.library", 1);
    }
    return JSONBase != NULL;
  }
} opener;
}  // namespace

#define DOC(d) ((CONST struct JSONDoc *)(d))
#define WR(w) ((struct JSONWriter *)(w))

namespace ojl {

bool Available() { return opener.Open(); }

Doc *Parse(const char *text, size_t len, unsigned flags, Error *err) {
  struct JSONError e = {0, 0, 0, 0};
  if (!opener.Open()) {
    if (err) *err = Error{-1, 0, 0, 0};
    return NULL;
  }
  struct JSONDoc *d = JSONParse((CONST_STRPTR)text, (ULONG)len, flags, &e);
  if (err) *err = Error{e.je_Code, e.je_Offset, e.je_Line, e.je_Column};
  return (Doc *)d;
}
void Free(Doc *doc) {
  if (doc) JSONFreeDoc((struct JSONDoc *)doc);
}
const char *ErrorString(long code) {
  if (code < 0) return "json.library is not installed";
  if (!opener.Open()) return "json.library is not installed";
  return (const char *)JSONErrorString(code);
}
unsigned Type(const Doc *d, unsigned n) { return JSONType(DOC(d), n); }
unsigned Count(const Doc *d, unsigned n) { return JSONCount(DOC(d), n); }
unsigned Skip(const Doc *d, unsigned n) { return JSONSkip(DOC(d), n); }
unsigned Find(const Doc *d, unsigned n, const char *k, size_t l) {
  return JSONFind(DOC(d), n, (CONST_STRPTR)k, (ULONG)l);
}
const char *String(const Doc *d, unsigned n, size_t *len) {
  ULONG l = 0;
  CONST_STRPTR s = JSONGetString(DOC(d), n, &l);
  if (len) *len = l;
  return (const char *)s;
}
bool Long(const Doc *d, unsigned n, long *v) {
  LONG x = 0;
  if (JSONGetLong(DOC(d), n, &x) != JSONERR_OK) return false;
  *v = x;
  return true;
}
bool Double(const Doc *d, unsigned n, double *v) {
  DOUBLE x = 0;
  if (JSONGetDouble(DOC(d), n, &x) != JSONERR_OK) return false;
  *v = x;
  return true;
}

Writer *NewWriter(unsigned flags) {
  if (!opener.Open()) return NULL;
  return (Writer *)JSONNewWriter(flags);
}
void FreeWriter(Writer *w) {
  if (w) JSONFreeWriter(WR(w));
}
void Begin(Writer *w, unsigned t) { JSONWriteBegin(WR(w), t); }
void End(Writer *w) { JSONWriteEnd(WR(w)); }
void Key(Writer *w, const char *k, size_t l) { JSONWriteKey(WR(w), (CONST_STRPTR)k, (ULONG)l); }
void Str(Writer *w, const char *s, size_t l) { JSONWriteString(WR(w), (CONST_STRPTR)s, (ULONG)l); }
void Long(Writer *w, long v) { JSONWriteLong(WR(w), v); }
void Double(Writer *w, double v) {
  DOUBLE x = v;
  JSONWriteDouble(WR(w), &x);
}
void Bool(Writer *w, bool v) { JSONWriteBool(WR(w), v ? 1 : 0); }
void Null(Writer *w) { JSONWriteNull(WR(w)); }
void NumberText(Writer *w, const char *t, size_t l) {
  JSONWriteNumberText(WR(w), (CONST_STRPTR)t, (ULONG)l);
}
const char *Text(Writer *w, size_t *len) {
  ULONG l = 0;
  if (JSONWriterError(WR(w)) != JSONERR_OK) return NULL;
  CONST_STRPTR s = JSONWriterText(WR(w), &l);
  if (len) *len = l;
  return (const char *)s;
}

}  // namespace ojl

#else  // the host check: json.library's core, the same code the library runs

#include <stdlib.h>
#include "json_core.h"

namespace {
void *h_alloc(struct oj_env *, oj_u32 n) { return malloc(n ? n : 1); }
void h_free(struct oj_env *, void *p) { free(p); }
struct oj_env env = {h_alloc, h_free};
/* the core keeps a double as the 68k does: high word first */
void swap(oj_u32 *w) {
  if (*(const unsigned short *)"\1" == 1) {
    oj_u32 t = w[0];
    w[0] = w[1];
    w[1] = t;
  }
}
}  // namespace

#define DOC(d) ((const struct oj_doc *)(d))
#define WR(w) ((struct oj_writer *)(w))

namespace ojl {

bool Available() { return true; }
Doc *Parse(const char *text, size_t len, unsigned flags, Error *err) {
  struct oj_error e = {};
  struct oj_doc *d = oj_parse(&env, (const oj_u8 *)text, (oj_u32)len, flags, &e);
  if (err) *err = Error{e.code, e.offset, e.line, e.column};
  return (Doc *)d;
}
void Free(Doc *doc) {
  if (doc) oj_free_doc(&env, (struct oj_doc *)doc);
}
const char *ErrorString(long code) { return oj_error_string(code); }
unsigned Type(const Doc *d, unsigned n) { return oj_type(DOC(d), n); }
unsigned Count(const Doc *d, unsigned n) { return oj_count(DOC(d), n); }
unsigned Skip(const Doc *d, unsigned n) { return oj_skip(DOC(d), n); }
unsigned Find(const Doc *d, unsigned n, const char *k, size_t l) {
  return oj_find(DOC(d), n, k, (oj_u32)l);
}
const char *String(const Doc *d, unsigned n, size_t *len) {
  oj_u32 l = 0;
  const char *s = oj_get_string(DOC(d), n, &l);
  if (len) *len = l;
  return s;
}
bool Long(const Doc *d, unsigned n, long *v) {
  oj_s32 x = 0;
  if (oj_get_long(DOC(d), n, &x) != 0) return false;
  *v = x;
  return true;
}
bool Double(const Doc *d, unsigned n, double *v) {
  oj_u32 w[2];
  if (oj_get_double(&env, DOC(d), n, w) != 0) return false;
  swap(w);
  memcpy(v, w, sizeof *v);
  return true;
}
Writer *NewWriter(unsigned flags) { return (Writer *)oj_new_writer(&env, flags); }
void FreeWriter(Writer *w) {
  if (w) oj_free_writer(WR(w));
}
void Begin(Writer *w, unsigned t) { oj_write_begin(WR(w), t); }
void End(Writer *w) { oj_write_end(WR(w)); }
void Key(Writer *w, const char *k, size_t l) { oj_write_key(WR(w), k, (oj_u32)l); }
void Str(Writer *w, const char *s, size_t l) { oj_write_string(WR(w), s, (oj_u32)l); }
void Long(Writer *w, long v) { oj_write_long(WR(w), (oj_s32)v); }
void Double(Writer *w, double v) {
  oj_u32 x[2];
  memcpy(x, &v, sizeof x);
  swap(x);
  oj_write_double(WR(w), x);
}
void Bool(Writer *w, bool v) { oj_write_bool(WR(w), v ? 1 : 0); }
void Null(Writer *w) { oj_write_null(WR(w)); }
void NumberText(Writer *w, const char *t, size_t l) { oj_write_number_text(WR(w), t, (oj_u32)l); }
const char *Text(Writer *w, size_t *len) {
  oj_u32 l = 0;
  if (oj_writer_error(WR(w)) != 0) return NULL;
  const char *s = oj_writer_text(WR(w), &l);
  if (len) *len = l;
  return s;
}

}  // namespace ojl

#endif
