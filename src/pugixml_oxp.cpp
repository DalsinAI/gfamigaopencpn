/* pugixml's calls (compat/include/pugixml.hpp) on OXP's tree.
 * Copyright (c) 2026 AmiCradle Team (GPL v2 or later, as OpenCPN) */
#include "pugixml.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sstream>
#include <string>

#include "oxp_dom.h"

namespace pugi {

namespace {

oxp_doc* Owner(oxp_node* n) { return n ? n->owner : nullptr; }

bool IsText(const oxp_node* n) { return n && (n->type == OXP_NODE_TEXT || n->type == OXP_NODE_CDATA); }

bool OnlySpace(const oxp_node* n) {
  for (size_t i = 0; i < n->len; i++)
    if (!strchr(" \t\r\n", n->text[i])) return false;
  return true;
}

// pugixml's parse_default: no white-space-only text, no comments, no PIs
void Tidy(oxp_node* n) {
  oxp_node* c = n->first;
  while (c) {
    oxp_node* next = c->next;
    if ((c->type == OXP_NODE_TEXT && OnlySpace(c)) || c->type == OXP_NODE_COMMENT ||
        c->type == OXP_NODE_PI)
      oxp_dom_detach(c);
    else if (c->type == OXP_NODE_ELEMENT)
      Tidy(c);
    c = next;
  }
}

oxp_dattr* FindAttr(oxp_node* el, const char* name) {
  if (!el || el->type != OXP_NODE_ELEMENT || !name) return nullptr;
  for (oxp_dattr* a = el->attrs; a; a = a->next)
    if (!strcmp(a->local, name)) return a;
  return nullptr;
}

void Escape(std::ostream& os, const char* s, size_t len, bool attr) {
  for (size_t i = 0; i < len; i++) {
    char c = s[i];
    switch (c) {
      case '&': os << "&amp;"; break;
      case '<': os << "&lt;"; break;
      case '>': os << "&gt;"; break;
      case '"':
        if (attr) os << "&quot;"; else os << c;
        break;
      case '\r':
        if (attr) os << "&#13;"; else os << c;
        break;
      case '\n':
        if (attr) os << "&#10;"; else os << c;
        break;
      default: os << c;
    }
  }
}

void Indent(std::ostream& os, const char* indent, unsigned depth, unsigned flags) {
  if (!(flags & format_indent) || (flags & format_raw)) return;
  while (depth--) os << indent;
}

// pugixml's layout: an element on a line of its own, its text inline
void Write(std::ostream& os, const oxp_node* n, const char* indent, unsigned flags, unsigned depth) {
  bool pretty = (flags & format_indent) && !(flags & format_raw);
  switch (n->type) {
    case OXP_NODE_DOCUMENT:
      for (const oxp_node* c = n->first; c; c = c->next) Write(os, c, indent, flags, depth);
      return;
    case OXP_NODE_TEXT:
      Escape(os, n->text, n->len, false);
      return;
    case OXP_NODE_CDATA:
      os << "<![CDATA[";
      os.write(n->text, n->len);
      os << "]]>";
      return;
    case OXP_NODE_COMMENT:
      os << "<!--";
      os.write(n->text, n->len);
      os << "-->";
      return;
    case OXP_NODE_PI:
      os << "<?" << n->name << " ";
      os.write(n->text, n->len);
      os << "?>";
      return;
    case OXP_NODE_ELEMENT:
      break;
  }
  Indent(os, indent, depth, flags);
  os << '<' << n->name;
  for (const oxp_dattr* a = n->attrs; a; a = a->next) {
    os << ' ' << a->local << "=\"";
    Escape(os, a->value, a->value_len, true);
    os << '"';
  }
  if (!n->first) {
    os << " />";
    if (pretty) os << '\n';
    return;
  }
  bool textonly = true;
  for (const oxp_node* c = n->first; c; c = c->next)
    if (!IsText(c)) textonly = false;
  os << '>';
  if (textonly) {
    for (const oxp_node* c = n->first; c; c = c->next) Write(os, c, indent, flags, depth + 1);
  } else {
    if (pretty) os << '\n';
    for (const oxp_node* c = n->first; c; c = c->next) {
      if (IsText(c)) {
        Indent(os, indent, depth + 1, flags);
        Write(os, c, indent, flags, depth + 1);
        if (pretty) os << '\n';
      } else
        Write(os, c, indent, flags, depth + 1);
    }
    Indent(os, indent, depth, flags);
  }
  os << "</" << n->name << '>';
  if (pretty) os << '\n';
}

bool HasDeclaration(const oxp_node* doc) {
  for (const oxp_node* c = doc ? doc->first : nullptr; c; c = c->next)
    if (c->type == OXP_NODE_PI && c->name && !strcmp(c->name, "xml")) return true;
  return false;
}

std::string Number(long long v) {
  char b[32];
  snprintf(b, sizeof b, "%lld", v);
  return b;
}
std::string Number(unsigned long long v) {
  char b[32];
  snprintf(b, sizeof b, "%llu", v);
  return b;
}

}  // namespace

const char* xml_parse_result::description() const {
  switch (status) {
    case status_ok: return "No error";
    case status_file_not_found: return "File was not found";
    case status_io_error: return "Error reading from file/stream";
    case status_out_of_memory: return "Could not allocate memory";
    case status_no_document_element: return "No document element found";
    default: return "Error parsing document";
  }
}

// ---- attributes

const char* xml_attribute::name() const { return a_ ? a_->local : ""; }
const char* xml_attribute::value() const { return a_ && a_->value ? a_->value : ""; }
int xml_attribute::as_int(int def) const { return a_ ? (int)strtol(value(), nullptr, 0) : def; }
unsigned int xml_attribute::as_uint(unsigned int def) const {
  return a_ ? (unsigned int)strtoul(value(), nullptr, 0) : def;
}
long long xml_attribute::as_llong(long long def) const { return a_ ? strtoll(value(), nullptr, 0) : def; }
double xml_attribute::as_double(double def) const { return a_ ? strtod(value(), nullptr) : def; }
bool xml_attribute::as_bool(bool def) const {
  if (!a_) return def;
  char c = value()[0];
  return c == '1' || c == 't' || c == 'T' || c == 'y' || c == 'Y';
}

bool xml_attribute::set_value(const char* v) {
  if (!a_) return false;
  std::string name = a_->local;
  if (oxp_dom_set_attr(el_, nullptr, nullptr, name.c_str(), v ? v : "", 0) != 0) return false;
  a_ = FindAttr(el_, name.c_str());
  return a_ != nullptr;
}
bool xml_attribute::set_value(int v) { return set_value(Number((long long)v).c_str()); }
bool xml_attribute::set_value(unsigned int v) { return set_value(Number((unsigned long long)v).c_str()); }
bool xml_attribute::set_value(long v) { return set_value(Number((long long)v).c_str()); }
bool xml_attribute::set_value(unsigned long v) { return set_value(Number((unsigned long long)v).c_str()); }
bool xml_attribute::set_value(long long v) { return set_value(Number(v).c_str()); }
bool xml_attribute::set_value(unsigned long long v) { return set_value(Number(v).c_str()); }
bool xml_attribute::set_value(double v) {
  char b[40];
  snprintf(b, sizeof b, "%.17g", v);  // pugixml's precision
  return set_value(b);
}
bool xml_attribute::set_name(const char* n) {
  if (!a_ || !n) return false;
  std::string val = value(), old = a_->local;
  oxp_dom_remove_attr(el_, nullptr, old.c_str());
  if (oxp_dom_set_attr(el_, nullptr, nullptr, n, val.c_str(), 0) != 0) return false;
  a_ = FindAttr(el_, n);
  return a_ != nullptr;
}
xml_attribute xml_attribute::next_attribute() const {
  return a_ ? xml_attribute(el_, a_->next) : xml_attribute();
}
xml_attribute xml_attribute::previous_attribute() const {
  if (!a_) return xml_attribute();
  oxp_dattr* p = nullptr;
  for (oxp_dattr* a = el_->attrs; a && a != a_; a = a->next) p = a;
  return xml_attribute(el_, p);
}

// ---- nodes

xml_node_type xml_node::type() const {
  if (!n_) return node_null;
  switch (n_->type) {
    case OXP_NODE_DOCUMENT: return node_document;
    case OXP_NODE_ELEMENT: return node_element;
    case OXP_NODE_TEXT: return node_pcdata;
    case OXP_NODE_CDATA: return node_cdata;
    case OXP_NODE_COMMENT: return node_comment;
    case OXP_NODE_PI: return node_pi;
  }
  return node_null;
}
const char* xml_node::name() const {
  if (!n_ || !(n_->type == OXP_NODE_ELEMENT || n_->type == OXP_NODE_PI) || !n_->name) return "";
  return n_->name;
}
const char* xml_node::value() const {
  if (!n_ || n_->type == OXP_NODE_ELEMENT || n_->type == OXP_NODE_DOCUMENT || !n_->text) return "";
  return n_->text;
}
const char* xml_node::child_value() const {
  if (!n_) return "";
  for (oxp_node* c = n_->first; c; c = c->next)
    if (IsText(c)) return c->text ? c->text : "";
  return "";
}
xml_node xml_node::parent() const { return xml_node(n_ ? n_->parent : nullptr); }
xml_node xml_node::first_child() const { return xml_node(n_ ? n_->first : nullptr); }
xml_node xml_node::last_child() const { return xml_node(n_ ? n_->last : nullptr); }
xml_node xml_node::next_sibling() const { return xml_node(n_ ? n_->next : nullptr); }
xml_node xml_node::previous_sibling() const { return xml_node(n_ ? n_->prev : nullptr); }
xml_node xml_node::next_sibling(const char* nm) const {
  for (oxp_node* c = n_ ? n_->next : nullptr; c; c = c->next)
    if (c->type == OXP_NODE_ELEMENT && c->name && nm && !strcmp(c->name, nm)) return xml_node(c);
  return xml_node();
}
xml_node xml_node::child(const char* nm) const {
  for (oxp_node* c = n_ ? n_->first : nullptr; c; c = c->next)
    if (c->type == OXP_NODE_ELEMENT && c->name && nm && !strcmp(c->name, nm)) return xml_node(c);
  return xml_node();
}
xml_node xml_node::root() const {
  oxp_node* n = n_;
  while (n && n->parent) n = n->parent;
  return xml_node(n);
}
xml_attribute xml_node::first_attribute() const {
  return (n_ && n_->type == OXP_NODE_ELEMENT) ? xml_attribute(n_, n_->attrs) : xml_attribute();
}
xml_attribute xml_node::last_attribute() const {
  oxp_dattr* l = nullptr;
  for (oxp_dattr* a = (n_ && n_->type == OXP_NODE_ELEMENT) ? n_->attrs : nullptr; a; a = a->next) l = a;
  return xml_attribute(n_, l);
}
xml_attribute xml_node::attribute(const char* nm) const { return xml_attribute(n_, FindAttr(n_, nm)); }
xml_text xml_node::text() const { return xml_text(n_); }

bool xml_node::set_value(const char* v) {
  if (!n_ || n_->type == OXP_NODE_ELEMENT || n_->type == OXP_NODE_DOCUMENT) return false;
  return oxp_dom_set_text(n_, v ? v : "") == 0;
}
bool xml_node::set_name(const char* nm) {
  // OXP names an element when it is made: a new element takes this one's
  // place, with its attributes and children
  if (!n_ || n_->type != OXP_NODE_ELEMENT || !nm || !n_->parent) return false;
  oxp_node* e = oxp_dom_create(Owner(n_), OXP_NODE_ELEMENT, nullptr, nullptr, nm, nullptr);
  if (!e) return false;
  for (oxp_dattr* a = n_->attrs; a; a = a->next) oxp_dom_set_attr(e, nullptr, nullptr, a->local, a->value, 0);
  while (n_->first) {
    oxp_node* c = n_->first;
    oxp_dom_detach(c);
    oxp_dom_insert(e, c, nullptr);
  }
  oxp_node* parent = n_->parent;
  oxp_node* next = n_->next;
  oxp_dom_detach(n_);
  oxp_dom_insert(parent, e, next);
  n_ = e;
  return true;
}
xml_attribute xml_node::append_attribute(const char* nm) {
  if (!n_ || n_->type != OXP_NODE_ELEMENT || !nm) return xml_attribute();
  if (oxp_dom_set_attr(n_, nullptr, nullptr, nm, "", 0) != 0) return xml_attribute();
  return xml_attribute(n_, FindAttr(n_, nm));
}
xml_node xml_node::append_child(xml_node_type t) {
  if (!n_) return xml_node();
  oxp_node_type ot;
  switch (t) {
    case node_element: ot = OXP_NODE_ELEMENT; break;
    case node_pcdata: ot = OXP_NODE_TEXT; break;
    case node_cdata: ot = OXP_NODE_CDATA; break;
    case node_comment: ot = OXP_NODE_COMMENT; break;
    default: return xml_node();
  }
  oxp_node* c = oxp_dom_create(Owner(n_), ot, nullptr, nullptr, ot == OXP_NODE_ELEMENT ? "" : nullptr,
                               ot == OXP_NODE_ELEMENT ? nullptr : "");
  if (!c || oxp_dom_insert(n_, c, nullptr) != 0) return xml_node();
  return xml_node(c);
}
xml_node xml_node::append_child(const char* nm) {
  if (!n_ || !nm) return xml_node();
  oxp_node* c = oxp_dom_create(Owner(n_), OXP_NODE_ELEMENT, nullptr, nullptr, nm, nullptr);
  if (!c || oxp_dom_insert(n_, c, nullptr) != 0) return xml_node();
  return xml_node(c);
}
xml_node xml_node::prepend_child(const char* nm) {
  if (!n_ || !nm) return xml_node();
  oxp_node* c = oxp_dom_create(Owner(n_), OXP_NODE_ELEMENT, nullptr, nullptr, nm, nullptr);
  if (!c || oxp_dom_insert(n_, c, n_->first) != 0) return xml_node();
  return xml_node(c);
}
bool xml_node::remove_child(const xml_node& c) {
  if (!n_ || !c.n_ || c.n_->parent != n_) return false;
  return oxp_dom_detach(c.n_) == 0;
}
bool xml_node::remove_attribute(const char* nm) {
  if (!n_ || !nm) return false;
  return oxp_dom_remove_attr(n_, nullptr, nm) == 0;
}
void xml_node::print(std::ostream& os, const char* indent, unsigned int flags, xml_encoding,
                     unsigned int depth) const {
  if (n_) Write(os, n_, indent ? indent : "\t", flags, depth);
}

// ---- text

oxp_node* xml_text::Data() const {
  if (!el_) return nullptr;
  if (IsText(el_)) return el_;
  for (oxp_node* c = el_->first; c; c = c->next)
    if (IsText(c)) return c;
  return nullptr;
}
const char* xml_text::get() const {
  oxp_node* d = Data();
  return d && d->text ? d->text : "";
}
int xml_text::as_int(int def) const { return Data() ? (int)strtol(get(), nullptr, 0) : def; }
double xml_text::as_double(double def) const { return Data() ? strtod(get(), nullptr) : def; }
bool xml_text::as_bool(bool def) const {
  if (!Data()) return def;
  char c = get()[0];
  return c == '1' || c == 't' || c == 'T' || c == 'y' || c == 'Y';
}
bool xml_text::set(const char* v) {
  oxp_node* d = Data();
  if (!d) {
    if (!el_ || el_->type != OXP_NODE_ELEMENT) return false;
    d = oxp_dom_create(el_->owner, OXP_NODE_TEXT, nullptr, nullptr, nullptr, "");
    if (!d || oxp_dom_insert(el_, d, nullptr) != 0) return false;
  }
  return oxp_dom_set_text(d, v ? v : "") == 0;
}

// ---- documents

xml_document::xml_document() {
  doc_ = oxp_dom_new();
  n_ = doc_ ? oxp_dom_document(doc_) : nullptr;
}
xml_document::~xml_document() {
  if (doc_) oxp_dom_free(doc_);
}
void xml_document::reset() {
  if (doc_) oxp_dom_free(doc_);
  doc_ = oxp_dom_new();
  n_ = doc_ ? oxp_dom_document(doc_) : nullptr;
}
xml_parse_result xml_document::load_buffer(const void* contents, size_t size, unsigned int, xml_encoding) {
  xml_parse_result r;
  oxp_code code = (oxp_code)0;
  unsigned long line = 0, column = 0;
  oxp_doc* d = oxp_dom_parse(contents, size, OXP_OPT_NO_NAMESPACES, &code, nullptr, 0, &line, &column);
  if (!d) {
    r.status = status_bad_pcdata;
    r.offset = 0;
    return r;  // the document is left as it was, as pugixml does on an error
  }
  if (doc_) oxp_dom_free(doc_);
  doc_ = d;
  n_ = oxp_dom_document(doc_);
  Tidy(n_);
  r.status = oxp_dom_root(doc_) ? status_ok : status_no_document_element;
  return r;
}
xml_parse_result xml_document::load_string(const char* contents, unsigned int options) {
  return load_buffer(contents, contents ? strlen(contents) : 0, options);
}
xml_parse_result xml_document::load_file(const char* path, unsigned int options, xml_encoding enc) {
  xml_parse_result r;
  FILE* f = path ? fopen(path, "rb") : nullptr;
  if (!f) {
    r.status = status_file_not_found;
    return r;
  }
  std::string data;
  char buf[8192];
  size_t got;
  while ((got = fread(buf, 1, sizeof buf, f)) > 0) data.append(buf, got);
  bool bad = ferror(f) != 0;
  fclose(f);
  if (bad) {
    r.status = status_io_error;
    return r;
  }
  return load_buffer(data.data(), data.size(), options, enc);
}
void xml_document::save(std::ostream& os, const char* indent, unsigned int flags, xml_encoding) const {
  if (!(flags & format_no_declaration) && !HasDeclaration(n_)) {
    os << "<?xml version=\"1.0\"?>";
    if ((flags & format_indent) && !(flags & format_raw)) os << '\n';
  }
  if (n_) Write(os, n_, indent ? indent : "\t", flags, 0);
}
bool xml_document::save_file(const char* path, const char* indent, unsigned int flags,
                             xml_encoding enc) const {
  std::ostringstream ss;
  save(ss, indent, flags, enc);
  FILE* f = path ? fopen(path, "wb") : nullptr;
  if (!f) return false;
  const std::string& s = ss.str();
  bool ok = fwrite(s.data(), 1, s.size(), f) == s.size();
  ok = (fclose(f) == 0) && ok;
  return ok;
}
xml_node xml_document::document_element() const { return xml_node(doc_ ? oxp_dom_root(doc_) : nullptr); }

}  // namespace pugi
