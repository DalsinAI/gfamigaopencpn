/* rapidjson::StringBuffer for OpenCPN on json.library (see document.h).
 * Copyright (c) 2026 AmiCradle Team (GPL v2 or later, as OpenCPN) */
#ifndef OCPN_RAPIDJSON_STRINGBUFFER_H
#define OCPN_RAPIDJSON_STRINGBUFFER_H
#include <string>
#include "fwd.h"
namespace rapidjson {
class StringBuffer {
public:
  const char* GetString() const { return s_.c_str(); }
  size_t GetSize() const { return s_.size(); }
  void Clear() { s_.clear(); }
  std::string s_;
};
}  // namespace rapidjson
#endif
