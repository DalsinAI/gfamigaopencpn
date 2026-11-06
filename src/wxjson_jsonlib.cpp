/* wxJSONReader and wxJSONWriter (OpenCPN's libs/wxJSON) on json.library:
 * the text is parsed and written by GFOpenJSON, and this file only moves
 * values between its documents and wxJSONValue. It takes the place of
 * libs/wxJSON/src/jsonreader.cpp and jsonwriter.cpp, with their headers and
 * public calls as they are; jsonval.cpp (the value class) is OpenCPN's.
 * Copyright (c) 2026 AmiCradle Team (GPL v2 or later, as OpenCPN) */
#include <wx/jsonreader.h>
#include <wx/jsonwriter.h>
#include <wx/mstream.h>
#include <wx/sstream.h>
#include <wx/stream.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include "ocpn_json.h"

// ---- reading

wxJSONReader::wxJSONReader(int flags, int maxErrors)
    : m_flags(flags),
      m_maxErrors(maxErrors),
      m_lineNo(1),
      m_colNo(1),
      m_level(0),
      m_depth(0),
      m_current(NULL),
      m_lastStored(NULL),
      m_next(NULL),
      m_commentLine(-1),
      m_peekChar(-1),
      m_noUtf8((flags & wxJSONREADER_NOUTF8_STREAM) != 0) {}

wxJSONReader::~wxJSONReader() {}

void wxJSONReader::AddError(const wxString& descr) {
  if ((int)m_errors.GetCount() < m_maxErrors) m_errors.Add(descr);
}

// One value of the document (node n) into val; returns the node after it.
static unsigned ReadNode(const ojl::Doc* d, unsigned n, wxJSONValue& val,
                         int level, int& depth) {
  if (level > depth) depth = level;
  size_t len = 0;
  const char* s;
  switch (ojl::Type(d, n)) {
    case ojl::NUL:
      val = wxJSONValue(wxJSONTYPE_NULL);
      return n + 1;
    case ojl::TRUE_:
      val = wxJSONValue(true);
      return n + 1;
    case ojl::FALSE_:
      val = wxJSONValue(false);
      return n + 1;
    case ojl::INT: {
      long v = 0;
      ojl::Long(d, n, &v);
      val = wxJSONValue((int)v);
      return n + 1;
    }
    case ojl::NUMBER: {
      // wxJSON's own reader: an integer as the smallest type that holds it
      // (int, 64-bit, unsigned 64-bit), anything else a double
      s = ojl::NumberText(d, n, &len);
      std::string t(s ? s : "0", s ? len : 1);
      if (t.find_first_of(".eE") == std::string::npos) {
        errno = 0;
        char* end = NULL;
        long long ll = strtoll(t.c_str(), &end, 10);
        if (errno == 0 && end && *end == 0) {
          val = wxJSONValue((wxInt64)ll);
          return n + 1;
        }
        if (t[0] != '-') {
          errno = 0;
          unsigned long long ull = strtoull(t.c_str(), &end, 10);
          if (errno == 0 && end && *end == 0) {
            val = wxJSONValue((wxUint64)ull);
            return n + 1;
          }
        }
      }
      double dv = 0;
      ojl::Double(d, n, &dv);
      val = wxJSONValue(dv);
      return n + 1;
    }
    case ojl::STRING:
      s = ojl::String(d, n, &len);
      val = wxJSONValue(wxString::FromUTF8(s ? s : "", len));
      return n + 1;
    case ojl::ARRAY: {
      unsigned count = ojl::Count(d, n), c = n + 1;
      val = wxJSONValue(wxJSONTYPE_ARRAY);
      for (unsigned i = 0; i < count; i++) {
        wxJSONValue item;
        c = ReadNode(d, c, item, level + 1, depth);
        val.Append(item);
      }
      return ojl::Skip(d, n);
    }
    case ojl::OBJECT: {
      unsigned count = ojl::Count(d, n), c = n + 1;
      val = wxJSONValue(wxJSONTYPE_OBJECT);
      for (unsigned i = 0; i < count; i++) {
        s = ojl::String(d, c, &len);
        wxString key = wxString::FromUTF8(s ? s : "", len);
        wxJSONValue item;
        c = ReadNode(d, c + 1, item, level + 1, depth);
        val[key] = item;  // the last of equal keys, as wxJSON keeps
      }
      return ojl::Skip(d, n);
    }
  }
  return ojl::Skip(d, n);
}

int wxJSONReader::DoRead(wxInputStream& is, wxJSONValue& val) {
  std::string text;
  char buf[4096];
  while (is.CanRead()) {
    is.Read(buf, sizeof buf);
    size_t got = is.LastRead();
    if (got == 0) break;
    text.append(buf, got);
  }
  m_errors.Clear();
  m_warnings.Clear();
  m_depth = 0;
  unsigned flags = ojl::F_TRAILING;
  if (m_flags & wxJSONREADER_ALLOW_COMMENTS) flags |= ojl::F_COMMENTS;
  if (m_noUtf8) flags |= ojl::F_LATIN1;
  ojl::Error err = {0, 0, 0, 0};
  ojl::Doc* d = ojl::Parse(text.data(), text.size(), flags, &err);
  if (!d) {
    m_lineNo = (int)err.line;
    m_colNo = (int)err.column;
    AddError(wxString::Format("Error: line %d, col %d - %s", m_lineNo, m_colNo,
                              ojl::ErrorString(err.code)));
    return (int)m_errors.GetCount();
  }
  ReadNode(d, 0, val, 1, m_depth);
  ojl::Free(d);
  return 0;
}

int wxJSONReader::Parse(const wxString& doc, wxJSONValue* val) {
  wxJSONValue temp;
  if (!val) val = &temp;
  wxScopedCharBuffer utf8 = doc.ToUTF8();
  wxMemoryInputStream is(utf8.data(), utf8.length());
  bool latin = m_noUtf8;
  m_noUtf8 = false;  // a wxString is always given as UTF-8
  int r = DoRead(is, *val);
  m_noUtf8 = latin;
  return r;
}

int wxJSONReader::Parse(wxInputStream& is, wxJSONValue* val) {
  wxJSONValue temp;
  if (!val) val = &temp;
  return DoRead(is, *val);
}

int wxJSONReader::GetDepth() const { return m_depth; }
int wxJSONReader::GetErrorCount() const { return (int)m_errors.GetCount(); }
int wxJSONReader::GetWarningCount() const { return (int)m_warnings.GetCount(); }
const wxArrayString& wxJSONReader::GetErrors() const { return m_errors; }
const wxArrayString& wxJSONReader::GetWarnings() const { return m_warnings; }

// ---- writing

wxJSONWriter::wxJSONWriter(int style, int indent, int step)
    : m_style(style),
      m_indent(indent),
      m_step(step),
      m_level(0),
      m_lineNo(0),
      m_colNo(0),
      m_noUtf8((style & wxJSONWRITER_NOUTF8_STREAM) != 0),
      m_fmt(NULL) {}

wxJSONWriter::~wxJSONWriter() { free(m_fmt); }

void wxJSONWriter::SetDoubleFmtString(const char* fmt) {
  // json.library writes the shortest text that reads back as the same
  // double; the format is kept only for the caller's sake
  free(m_fmt);
  m_fmt = fmt ? strdup(fmt) : NULL;
}

static void WriteUtf8(ojl::Writer* w, const wxString& s, bool key) {
  wxScopedCharBuffer u = s.ToUTF8();
  if (key)
    ojl::Key(w, u.data(), u.length());
  else
    ojl::Str(w, u.data(), u.length());
}

static void WriteValue(ojl::Writer* w, const wxJSONValue& v) {
  char num[32];
  switch (v.GetType()) {
    case wxJSONTYPE_INVALID:
    case wxJSONTYPE_NULL:
      ojl::Null(w);
      break;
    case wxJSONTYPE_INT:
    case wxJSONTYPE_SHORT:
    case wxJSONTYPE_LONG:
    case wxJSONTYPE_INT64: {
      wxInt64 x = v.AsInt64();
      if (x >= -2147483647LL - 1 && x <= 2147483647LL)
        ojl::Long(w, (long)x);
      else {
        snprintf(num, sizeof num, "%lld", (long long)x);
        ojl::NumberText(w, num, strlen(num));
      }
      break;
    }
    case wxJSONTYPE_UINT:
    case wxJSONTYPE_USHORT:
    case wxJSONTYPE_ULONG:
    case wxJSONTYPE_UINT64: {
      wxUint64 x = v.AsUInt64();
      if (x <= 2147483647ULL)
        ojl::Long(w, (long)x);
      else {
        snprintf(num, sizeof num, "%llu", (unsigned long long)x);
        ojl::NumberText(w, num, strlen(num));
      }
      break;
    }
    case wxJSONTYPE_DOUBLE:
      ojl::Double(w, v.AsDouble());
      break;
    case wxJSONTYPE_STRING:
    case wxJSONTYPE_CSTRING:
      WriteUtf8(w, v.AsString(), false);
      break;
    case wxJSONTYPE_BOOL:
      ojl::Bool(w, v.AsBool());
      break;
    case wxJSONTYPE_ARRAY: {
      ojl::Begin(w, ojl::ARRAY);
      int n = v.Size();
      for (int i = 0; i < n; i++) WriteValue(w, v.ItemAt((unsigned)i));
      ojl::End(w);
      break;
    }
    case wxJSONTYPE_OBJECT: {
      ojl::Begin(w, ojl::OBJECT);
      wxArrayString keys = v.GetMemberNames();
      for (size_t i = 0; i < keys.GetCount(); i++) {
        WriteUtf8(w, keys[i], true);
        WriteValue(w, v.ItemAt(keys[i]));
      }
      ojl::End(w);
      break;
    }
    case wxJSONTYPE_MEMORYBUFF: {
      // wxJSON's form: the bytes as two hex digits each, in a string
      wxMemoryBuffer b = v.AsMemoryBuff();
      std::string hex;
      const unsigned char* p = (const unsigned char*)b.GetData();
      for (size_t i = 0; i < b.GetDataLen(); i++) {
        snprintf(num, sizeof num, "%02X", p[i]);
        hex += num;
      }
      ojl::Str(w, hex.data(), hex.size());
      break;
    }
  }
}

// The value as JSON text (UTF-8), or false when json.library is missing
static bool WriteText(int style, const wxJSONValue& value, std::string& out) {
  unsigned flags = 0;
  if ((style & wxJSONWRITER_STYLED) && !(style & wxJSONWRITER_NO_LINEFEEDS))
    flags = (style & wxJSONWRITER_TAB_INDENT) ? ojl::W_TABS : ojl::W_PRETTY;
  ojl::Writer* w = ojl::NewWriter(flags);
  if (!w) return false;
  if (value.IsArray() || value.IsObject())
    WriteValue(w, value);
  else {  // json.library writes one whole value; wxJSON writes scalars too
    ojl::Begin(w, ojl::ARRAY);
    WriteValue(w, value);
    ojl::End(w);
  }
  size_t len = 0;
  const char* t = ojl::Text(w, &len);
  bool ok = t != NULL;
  if (ok) {
    out.assign(t, len);
    if (!(value.IsArray() || value.IsObject())) {
      // drop the array around the scalar
      size_t a = out.find('['), b = out.rfind(']');
      if (a != std::string::npos && b != std::string::npos && b > a)
        out = out.substr(a + 1, b - a - 1);
      size_t s = out.find_first_not_of(" \t\r\n"), e = out.find_last_not_of(" \t\r\n");
      out = s == std::string::npos ? std::string() : out.substr(s, e - s + 1);
    }
    if (flags) out += '\n';
  }
  ojl::FreeWriter(w);
  return ok;
}

void wxJSONWriter::Write(const wxJSONValue& value, wxString& str) {
  std::string t;
  if (WriteText(m_style, value, t)) str = wxString::FromUTF8(t.data(), t.size());
}

void wxJSONWriter::Write(const wxJSONValue& value, wxOutputStream& os) {
  std::string t;
  if (!WriteText(m_style, value, t)) return;
  if (m_noUtf8) {  // the stream wants the local 8-bit text
    wxString s = wxString::FromUTF8(t.data(), t.size());
    wxCharBuffer b = s.mb_str(wxConvLibc);
    os.Write(b.data(), strlen(b.data()));
  } else
    os.Write(t.data(), t.size());
}
