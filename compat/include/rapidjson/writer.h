/* rapidjson::Writer for OpenCPN on json.library (see document.h): a value
 * is written out through json.library's writer by Value::Accept.
 * Copyright (c) 2026 AmiCradle Team (GPL v2 or later, as OpenCPN) */
#ifndef OCPN_RAPIDJSON_WRITER_H
#define OCPN_RAPIDJSON_WRITER_H
#include "stringbuffer.h"
namespace rapidjson {
template <typename OutputStream>
class Writer {
public:
  explicit Writer(OutputStream& os) : os_(os) {}
  OutputStream& os_;
};
}  // namespace rapidjson
#endif
