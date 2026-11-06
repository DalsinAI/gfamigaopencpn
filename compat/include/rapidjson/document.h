/* The part of rapidjson's DOM that OpenCPN reads Signal K with (its AIS and
 * NMEA decoders), on json.library: Document::Parse is JSONParse, and a Value
 * is a node of the parsed block. Read-only, as OpenCPN uses it; this header
 * takes the place of rapidjson's own for this port.
 * Copyright (c) 2026 AmiCradle Team (GPL v2 or later, as OpenCPN) */
#ifndef OCPN_RAPIDJSON_DOCUMENT_H
#define OCPN_RAPIDJSON_DOCUMENT_H

#include <string.h>
#include <memory>
#include <string>

#include "fwd.h"
#include "stringbuffer.h"
#include "writer.h"
#include "ocpn_json.h"

namespace rapidjson {

class ValueIter;
class MemberIter;

namespace internal {
struct Holder {
  ojl::Doc* doc = nullptr;
  ~Holder() { ojl::Free(doc); }
};
}  // namespace internal

class Value {
public:
  typedef ValueIter ConstValueIterator;
  typedef MemberIter ConstMemberIterator;
  typedef ValueIter ValueIterator;
  typedef MemberIter MemberIterator;

  Value() {}
  Value(std::shared_ptr<internal::Holder> h, unsigned n) : h_(std::move(h)), n_(n) {}

  bool IsNull() const { return T() == ojl::NUL || T() == 0; }
  bool IsFalse() const { return T() == ojl::FALSE_; }
  bool IsTrue() const { return T() == ojl::TRUE_; }
  bool IsBool() const { return IsTrue() || IsFalse(); }
  bool IsObject() const { return T() == ojl::OBJECT; }
  bool IsArray() const { return T() == ojl::ARRAY; }
  bool IsString() const { return T() == ojl::STRING; }
  bool IsNumber() const { return T() == ojl::INT || T() == ojl::NUMBER; }
  bool IsInt() const { long v; return T() == ojl::INT || (T() == ojl::NUMBER && IsIntegral() && ojl::Long(D(), n_, &v)); }
  bool IsUint() const { return IsInt() && GetInt() >= 0; }
  bool IsInt64() const { return IsNumber() && IsIntegral(); }
  bool IsDouble() const { return T() == ojl::NUMBER && !IsIntegral(); }

  bool GetBool() const { return IsTrue(); }
  int GetInt() const { long v = 0; ojl::Long(D(), n_, &v); return (int)v; }
  unsigned GetUint() const { return (unsigned)GetInt(); }
  long long GetInt64() const {
    if (T() == ojl::INT) return GetInt();
    size_t l = 0;
    const char* s = ojl::NumberText(D(), n_, &l);
    return s ? std::stoll(std::string(s, l)) : 0;
  }
  double GetDouble() const {
    if (T() == ojl::INT) return GetInt();
    double v = 0;
    ojl::Double(D(), n_, &v);
    return v;
  }
  const char* GetString() const {
    const char* s = ojl::String(D(), n_, nullptr);
    return s ? s : "";
  }
  // rapidjson: a value equals a string when it is that string
  bool operator==(const char* s) const {
    if (!IsString() || !s) return false;
    size_t l = 0;
    const char* v = ojl::String(D(), n_, &l);
    return v && strlen(s) == l && !memcmp(v, s, l);
  }
  bool operator!=(const char* s) const { return !(*this == s); }
  SizeType GetStringLength() const {
    size_t l = 0;
    ojl::String(D(), n_, &l);
    return (SizeType)l;
  }

  SizeType Size() const { return IsArray() ? ojl::Count(D(), n_) : 0; }
  bool Empty() const { return Size() == 0; }
  SizeType MemberCount() const { return IsObject() ? ojl::Count(D(), n_) : 0; }
  bool HasMember(const char* key) const { return Member(key) != ojl::NONE; }
  bool HasMember(const std::string& key) const { return HasMember(key.c_str()); }

  Value operator[](const char* key) const { return Value(h_, Member(key)); }
  Value operator[](const std::string& key) const { return (*this)[key.c_str()]; }
  Value operator[](SizeType i) const {
    if (!IsArray() || i >= Size()) return Value();
    unsigned c = n_ + 1;
    while (i--) c = ojl::Skip(D(), c);
    return Value(h_, c);
  }
  Value operator[](int i) const { return (*this)[(SizeType)i]; }

  ValueIter Begin() const;
  ValueIter End() const;
  MemberIter MemberBegin() const;
  MemberIter MemberEnd() const;

  // the value as JSON text, through json.library's writer
  template <typename OutputStream>
  bool Accept(Writer<OutputStream>& w) const {
    ojl::Writer* jw = ojl::NewWriter(0);
    if (!jw) return false;
    bool scalar = !IsArray() && !IsObject();
    if (scalar) ojl::Begin(jw, ojl::ARRAY);
    Write(jw, n_);
    if (scalar) ojl::End(jw);
    size_t l = 0;
    const char* t = ojl::Text(jw, &l);
    if (t) {
      std::string s(t, l);
      if (scalar && s.size() >= 2) s = s.substr(1, s.size() - 2);
      w.os_.s_ += s;
    }
    ojl::FreeWriter(jw);
    return t != nullptr;
  }

protected:
  friend class ValueIter;
  friend class MemberIter;
  const ojl::Doc* D() const { return h_ ? h_->doc : nullptr; }
  unsigned T() const { return (h_ && h_->doc && n_ != ojl::NONE) ? ojl::Type(h_->doc, n_) : 0; }
  bool IsIntegral() const {
    size_t l = 0;
    const char* s = ojl::NumberText(D(), n_, &l);
    if (!s) return T() == ojl::INT;
    for (size_t i = 0; i < l; i++)
      if (s[i] == '.' || s[i] == 'e' || s[i] == 'E') return false;
    return true;
  }
  unsigned Member(const char* key) const {
    if (!IsObject() || !key) return ojl::NONE;
    size_t l = 0;
    while (key[l]) l++;
    return ojl::Find(D(), n_, key, l);
  }
  unsigned Write(ojl::Writer* w, unsigned n) const {  // returns the node after n's subtree
    size_t l = 0;
    const char* s;
    switch (ojl::Type(D(), n)) {
      case ojl::NUL: ojl::Null(w); return n + 1;
      case ojl::TRUE_: ojl::Bool(w, true); return n + 1;
      case ojl::FALSE_: ojl::Bool(w, false); return n + 1;
      case ojl::INT: { long v = 0; ojl::Long(D(), n, &v); ojl::Long(w, v); return n + 1; }
      case ojl::NUMBER: s = ojl::NumberText(D(), n, &l); ojl::NumberText(w, s, l); return n + 1;
      case ojl::STRING: s = ojl::String(D(), n, &l); ojl::Str(w, s, l); return n + 1;
      case ojl::ARRAY: {
        unsigned c = n + 1, k = ojl::Count(D(), n);
        ojl::Begin(w, ojl::ARRAY);
        while (k--) c = Write(w, c);
        ojl::End(w);
        return ojl::Skip(D(), n);
      }
      case ojl::OBJECT: {
        unsigned c = n + 1, k = ojl::Count(D(), n);
        ojl::Begin(w, ojl::OBJECT);
        while (k--) {
          s = ojl::String(D(), c, &l);
          ojl::Key(w, s, l);
          c = Write(w, c + 1);
        }
        ojl::End(w);
        return ojl::Skip(D(), n);
      }
    }
    return n + 1;
  }
  std::shared_ptr<internal::Holder> h_;
  unsigned n_ = ojl::NONE;
};

class ValueIter {
public:
  ValueIter(std::shared_ptr<internal::Holder> h, unsigned n) : v_(std::move(h), n) {}
  const Value& operator*() const { return v_; }
  const Value* operator->() const { return &v_; }
  ValueIter& operator++() {
    v_.n_ = ojl::Skip(v_.D(), v_.n_);
    return *this;
  }
  bool operator==(const ValueIter& o) const { return v_.n_ == o.v_.n_; }
  bool operator!=(const ValueIter& o) const { return v_.n_ != o.v_.n_; }
private:
  Value v_;
};
struct Member_ {
  Value name, value;
};
class MemberIter {
public:
  MemberIter(std::shared_ptr<internal::Holder> h, unsigned n) : h_(std::move(h)), n_(n) { Load(); }
  const Member_& operator*() const { return m_; }
  const Member_* operator->() const { return &m_; }
  MemberIter& operator++() {
    n_ = ojl::Skip(h_->doc, n_ + 1);
    Load();
    return *this;
  }
  bool operator==(const MemberIter& o) const { return n_ == o.n_; }
  bool operator!=(const MemberIter& o) const { return n_ != o.n_; }
private:
  void Load() { m_.name = Value(h_, n_); m_.value = Value(h_, n_ + 1); }
  std::shared_ptr<internal::Holder> h_;
  unsigned n_;
  Member_ m_;
};


inline ValueIter Value::Begin() const { return ValueIter(h_, IsArray() ? n_ + 1 : 0); }
inline ValueIter Value::End() const { return ValueIter(h_, IsArray() ? ojl::Skip(D(), n_) : 0); }
inline MemberIter Value::MemberBegin() const { return MemberIter(h_, IsObject() ? n_ + 1 : 0); }
inline MemberIter Value::MemberEnd() const { return MemberIter(h_, IsObject() ? ojl::Skip(D(), n_) : 0); }

class Document : public Value {
public:
  Document() {}
  Document& Parse(const char* text, size_t len) {
    auto h = std::make_shared<internal::Holder>();
    ojl::Error e = {0, 0, 0, 0};
    h->doc = ojl::Parse(text, len, 0, &e);
    err_ = h->doc ? 0 : (e.code ? e.code : -1);
    offset_ = e.offset;
    h_ = h;
    n_ = h->doc ? 0 : ojl::NONE;
    return *this;
  }
  Document& Parse(const char* text) {
    size_t l = 0;
    while (text && text[l]) l++;
    return Parse(text ? text : "", l);
  }
  Document& Parse(const std::string& text) { return Parse(text.data(), text.size()); }
  bool HasParseError() const { return err_ != 0; }
  long GetParseError() const { return err_; }
  size_t GetErrorOffset() const { return offset_; }
private:
  long err_ = 0;
  size_t offset_ = 0;
};

}  // namespace rapidjson

#endif
