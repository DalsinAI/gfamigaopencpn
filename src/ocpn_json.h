/* json.library (GFOpenJSON) for OpenCPN's JSON code: the few calls the
 * wxJSON and rapidjson adapters use, in one place. On the Amiga they are
 * json.library's, opened on first use (LIBS:, then PROGDIR:Libs/); the host
 * check build calls the library's core (json_core.cpp) straight.
 * Copyright (c) 2026 AmiCradle Team (GPL v2 or later, as OpenCPN) */
#ifndef OCPN_JSON_H
#define OCPN_JSON_H

#include <stddef.h>

namespace ojl {

enum { NUL = 1, FALSE_ = 2, TRUE_ = 3, INT = 4, NUMBER = 5, STRING = 6, ARRAY = 7, OBJECT = 8 };
enum { F_COMMENTS = 1, F_TRAILING = 2, F_LATIN1 = 4 };
enum { W_PRETTY = 1, W_TABS = 2 };
const unsigned NONE = 0xFFFFFFFFu;

struct Doc;      /* a parsed document: json.library's JSONDoc */
struct Writer;   /* json.library's JSONWriter */

struct Error {
  long code;
  unsigned long offset, line, column;
};

bool Available();               /* json.library could be opened */
Doc *Parse(const char *text, size_t len, unsigned flags, Error *err);
void Free(Doc *doc);
const char *ErrorString(long code);

unsigned Type(const Doc *doc, unsigned node);
unsigned Count(const Doc *doc, unsigned node);    /* elements, or members */
unsigned Skip(const Doc *doc, unsigned node);     /* the node after its subtree */
unsigned Find(const Doc *doc, unsigned node, const char *key, size_t keylen);
const char *String(const Doc *doc, unsigned node, size_t *len);
const char *NumberText(const Doc *doc, unsigned node, size_t *len);  /* a NUMBER's exact text */
bool Long(const Doc *doc, unsigned node, long *value);
bool Double(const Doc *doc, unsigned node, double *value);

Writer *NewWriter(unsigned flags);
void FreeWriter(Writer *w);
void Begin(Writer *w, unsigned type);   /* ARRAY or OBJECT */
void End(Writer *w);
void Key(Writer *w, const char *key, size_t len);
void Str(Writer *w, const char *s, size_t len);
void Long(Writer *w, long v);
void Double(Writer *w, double v);
void Bool(Writer *w, bool v);
void Null(Writer *w);
void NumberText(Writer *w, const char *text, size_t len);
const char *Text(Writer *w, size_t *len);   /* NULL when a call was out of order */

}  // namespace ojl

#endif
