/* The part of pugixml that OpenCPN uses (GPX files, its config catalogue,
 * the chart symbol list, SVG sizes, catalogue parsing), on OXP, GFOpenXML's
 * own XML parser: a document is OXP's tree, read by oxp_dom_parse and
 * changed and written with OXP's calls. This header takes the place of
 * libs/pugixml/pugixml.hpp for this port; src/pugixml_oxp.cpp holds the
 * code.
 *
 * As pugixml's defaults: text that is only white space, comments and
 * processing instructions are not kept; CDATA is; names are taken whole,
 * prefixes and all.
 * Copyright (c) 2026 AmiCradle Team (GPL v2 or later, as OpenCPN) */
#ifndef OCPN_PUGIXML_OXP_HPP
#define OCPN_PUGIXML_OXP_HPP

#include <stddef.h>
#include <ostream>

struct oxp_node;
struct oxp_dattr;
struct oxp_doc;

#define PUGIXML_TEXT(t) t
#define PUGIXML_VERSION 1140

namespace pugi {

typedef char char_t;

enum xml_node_type {
  node_null,
  node_document,
  node_element,
  node_pcdata,
  node_cdata,
  node_comment,
  node_pi,
  node_declaration,
  node_doctype
};

const unsigned int parse_default = 0x0074;  // pugixml's value; OXP's defaults stand in for it
const unsigned int format_default = 0x01;
const unsigned int format_indent = 0x01;
const unsigned int format_raw = 0x04;
const unsigned int format_no_declaration = 0x08;

enum xml_encoding { encoding_auto, encoding_utf8 };

enum xml_parse_status {
  status_ok = 0,
  status_file_not_found,
  status_io_error,
  status_out_of_memory,
  status_internal_error,
  status_unrecognized_tag,
  status_bad_pi,
  status_bad_comment,
  status_bad_cdata,
  status_bad_doctype,
  status_bad_pcdata,
  status_bad_start_element,
  status_bad_attribute,
  status_bad_end_element,
  status_end_element_mismatch,
  status_append_invalid_root,
  status_no_document_element
};

struct xml_parse_result {
  xml_parse_status status = status_internal_error;
  ptrdiff_t offset = 0;
  xml_encoding encoding = encoding_utf8;
  operator bool() const { return status == status_ok; }
  const char* description() const;
};

class xml_node;

class xml_attribute {
public:
  xml_attribute() {}
  xml_attribute(oxp_node* el, oxp_dattr* a) : el_(el), a_(a) {}
  bool empty() const { return !a_; }
  operator bool() const { return a_ != nullptr; }
  bool operator!() const { return !a_; }
  bool operator==(const xml_attribute& o) const { return a_ == o.a_; }
  bool operator!=(const xml_attribute& o) const { return a_ != o.a_; }

  const char* name() const;
  const char* value() const;
  const char* as_string(const char* def = "") const { return a_ ? value() : def; }
  int as_int(int def = 0) const;
  unsigned int as_uint(unsigned int def = 0) const;
  double as_double(double def = 0) const;
  float as_float(float def = 0) const { return (float)as_double(def); }
  bool as_bool(bool def = false) const;
  long long as_llong(long long def = 0) const;

  bool set_name(const char* n);
  bool set_value(const char* v);
  bool set_value(int v);
  bool set_value(unsigned int v);
  bool set_value(long v);
  bool set_value(unsigned long v);
  bool set_value(long long v);
  bool set_value(unsigned long long v);
  bool set_value(double v);
  bool set_value(float v) { return set_value((double)v); }
  bool set_value(bool v) { return set_value(v ? "true" : "false"); }
  template <typename T>
  xml_attribute& operator=(T v) {
    set_value(v);
    return *this;
  }

  xml_attribute next_attribute() const;
  xml_attribute previous_attribute() const;

private:
  friend class xml_node;
  oxp_node* el_ = nullptr;
  oxp_dattr* a_ = nullptr;
};

class xml_text;

class xml_node {
public:
  xml_node() {}
  explicit xml_node(oxp_node* n) : n_(n) {}
  bool empty() const { return !n_; }
  operator bool() const { return n_ != nullptr; }
  bool operator!() const { return !n_; }
  bool operator==(const xml_node& o) const { return n_ == o.n_; }
  bool operator!=(const xml_node& o) const { return n_ != o.n_; }

  xml_node_type type() const;
  const char* name() const;
  const char* value() const;
  const char* child_value() const;
  const char* child_value(const char* name) const { return child(name).child_value(); }

  xml_node parent() const;
  xml_node first_child() const;
  xml_node last_child() const;
  xml_node next_sibling() const;
  xml_node next_sibling(const char* name) const;
  xml_node previous_sibling() const;
  xml_node child(const char* name) const;
  xml_node root() const;
  xml_attribute first_attribute() const;
  xml_attribute last_attribute() const;
  xml_attribute attribute(const char* name) const;
  xml_text text() const;

  bool set_name(const char* n);
  bool set_value(const char* v);
  xml_attribute append_attribute(const char* name);
  xml_node append_child(xml_node_type type = node_element);
  xml_node append_child(const char* name);
  xml_node prepend_child(const char* name);
  bool remove_child(const xml_node& n);
  bool remove_child(const char* name) { return remove_child(child(name)); }
  bool remove_attribute(const char* name);

  // the node and all it holds as XML text
  void print(std::ostream& os, const char* indent = "\t", unsigned int flags = format_default,
             xml_encoding encoding = encoding_auto, unsigned int depth = 0) const;

  oxp_node* internal_object() const { return n_; }

protected:
  oxp_node* n_ = nullptr;
};

class xml_text {
public:
  explicit xml_text(oxp_node* el) : el_(el) {}
  operator bool() const { return Data() != nullptr; }
  bool empty() const { return Data() == nullptr; }
  const char* get() const;
  const char* as_string(const char* def = "") const { return Data() ? get() : def; }
  int as_int(int def = 0) const;
  double as_double(double def = 0) const;
  bool as_bool(bool def = false) const;
  bool set(const char* v);

private:
  oxp_node* Data() const;
  oxp_node* el_;
};

class xml_document : public xml_node {
public:
  xml_document();
  virtual ~xml_document();
  xml_document(const xml_document&) = delete;
  xml_document& operator=(const xml_document&) = delete;

  void reset();
  xml_parse_result load_string(const char* contents, unsigned int options = parse_default);
  xml_parse_result load_buffer(const void* contents, size_t size, unsigned int options = parse_default,
                               xml_encoding encoding = encoding_auto);
  xml_parse_result load_file(const char* path, unsigned int options = parse_default,
                             xml_encoding encoding = encoding_auto);
  bool save_file(const char* path, const char* indent = "\t", unsigned int flags = format_default,
                 xml_encoding encoding = encoding_auto) const;
  void save(std::ostream& os, const char* indent = "\t", unsigned int flags = format_default,
            xml_encoding encoding = encoding_auto) const;
  xml_node document_element() const;

private:
  oxp_doc* doc_ = nullptr;
};

}  // namespace pugi

#endif
