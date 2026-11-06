/* TinyXML's calls (compat/include/tinyxml.h), the text read by OXP.
 * Copyright (c) 2026 AmiCradle Team (GPL v2 or later, as OpenCPN) */
#include "tinyxml.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "oxp_dom.h"

namespace {

bool Space(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

// TinyXML's condensed white space: runs to one space, none at either end
std::string Condense(const char* s, size_t len) {
  std::string out;
  bool pending = false;
  for (size_t i = 0; i < len; i++) {
    if (Space(s[i])) {
      pending = !out.empty();
      continue;
    }
    if (pending) out += ' ';
    pending = false;
    out += s[i];
  }
  return out;
}

void Take(const oxp_node* from, TiXmlNode* to) {
  for (const oxp_node* c = from->first; c; c = c->next) {
    switch (c->type) {
      case OXP_NODE_ELEMENT: {
        TiXmlElement* e = new TiXmlElement(c->name ? c->name : "");
        for (const oxp_dattr* a = c->attrs; a; a = a->next)
          e->SetAttribute(a->local, a->value ? a->value : "");
        to->LinkEndChild(e);
        Take(c, e);
        break;
      }
      case OXP_NODE_TEXT: {
        std::string t = Condense(c->text, c->len);
        if (!t.empty()) to->LinkEndChild(new TiXmlText(t));
        break;
      }
      case OXP_NODE_CDATA: {
        TiXmlText* t = new TiXmlText(std::string(c->text, c->len));
        t->SetCDATA(true);
        to->LinkEndChild(t);
        break;
      }
      default:  // comments and processing instructions are not kept
        break;
    }
  }
}

void Escape(std::string& out, const std::string& s) {
  for (char c : s) {
    switch (c) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += "&quot;"; break;
      case '\'': out += "&apos;"; break;
      default:
        if ((unsigned char)c < 32 && c != '\n' && c != '\r' && c != '\t') {
          char b[8];
          snprintf(b, sizeof b, "&#x%02X;", (unsigned)(unsigned char)c);
          out += b;
        } else
          out += c;
    }
  }
}

bool Named(const TiXmlNode* n, const char* value) { return value && !strcmp(n->Value(), value); }

}  // namespace

// ---- nodes

TiXmlNode::~TiXmlNode() { Clear(); }

void TiXmlNode::Clear() {
  TiXmlNode* c = first_;
  while (c) {
    TiXmlNode* next = c->next_;
    delete c;
    c = next;
  }
  first_ = last_ = nullptr;
}

TiXmlNode* TiXmlNode::LinkEndChild(TiXmlNode* add) {
  if (!add) return nullptr;
  if (add->type_ == TINYXML_DOCUMENT) {
    delete add;
    return nullptr;
  }
  add->parent_ = this;
  add->prev_ = last_;
  add->next_ = nullptr;
  if (last_)
    last_->next_ = add;
  else
    first_ = add;
  last_ = add;
  return add;
}

TiXmlNode* TiXmlNode::InsertEndChild(const TiXmlNode& add) { return LinkEndChild(add.Clone()); }

bool TiXmlNode::RemoveChild(TiXmlNode* r) {
  if (!r || r->parent_ != this) return false;
  if (r->prev_)
    r->prev_->next_ = r->next_;
  else
    first_ = r->next_;
  if (r->next_)
    r->next_->prev_ = r->prev_;
  else
    last_ = r->prev_;
  delete r;
  return true;
}

void TiXmlNode::CopyChildren(TiXmlNode* to) const {
  for (const TiXmlNode* c = first_; c; c = c->next_) to->LinkEndChild(c->Clone());
}

TiXmlNode* TiXmlNode::FirstChild(const char* v) {
  for (TiXmlNode* c = first_; c; c = c->next_)
    if (Named(c, v)) return c;
  return nullptr;
}
const TiXmlNode* TiXmlNode::FirstChild(const char* v) const {
  return const_cast<TiXmlNode*>(this)->FirstChild(v);
}
TiXmlNode* TiXmlNode::NextSibling(const char* v) {
  for (TiXmlNode* c = next_; c; c = c->next_)
    if (Named(c, v)) return c;
  return nullptr;
}
TiXmlElement* TiXmlNode::FirstChildElement() {
  for (TiXmlNode* c = first_; c; c = c->next_)
    if (c->ToElement()) return c->ToElement();
  return nullptr;
}
const TiXmlElement* TiXmlNode::FirstChildElement() const {
  return const_cast<TiXmlNode*>(this)->FirstChildElement();
}
TiXmlElement* TiXmlNode::FirstChildElement(const char* v) {
  for (TiXmlNode* c = first_; c; c = c->next_)
    if (c->ToElement() && Named(c, v)) return c->ToElement();
  return nullptr;
}
const TiXmlElement* TiXmlNode::FirstChildElement(const char* v) const {
  return const_cast<TiXmlNode*>(this)->FirstChildElement(v);
}
TiXmlElement* TiXmlNode::NextSiblingElement() {
  for (TiXmlNode* c = next_; c; c = c->next_)
    if (c->ToElement()) return c->ToElement();
  return nullptr;
}
const TiXmlElement* TiXmlNode::NextSiblingElement() const {
  return const_cast<TiXmlNode*>(this)->NextSiblingElement();
}
TiXmlElement* TiXmlNode::NextSiblingElement(const char* v) {
  for (TiXmlNode* c = next_; c; c = c->next_)
    if (c->ToElement() && Named(c, v)) return c->ToElement();
  return nullptr;
}
const TiXmlElement* TiXmlNode::NextSiblingElement(const char* v) const {
  return const_cast<TiXmlNode*>(this)->NextSiblingElement(v);
}

// ---- elements

TiXmlElement::TiXmlElement(const char* v) : TiXmlNode(TINYXML_ELEMENT) { SetValue(v); }
TiXmlElement::TiXmlElement(const std::string& v) : TiXmlNode(TINYXML_ELEMENT) { SetValue(v); }
TiXmlElement::~TiXmlElement() {
  while (attrs_) {
    TiXmlAttr* n = attrs_->next;
    delete attrs_;
    attrs_ = n;
  }
}
TiXmlAttr* TiXmlElement::Find(const char* name) const {
  for (TiXmlAttr* a = attrs_; a; a = a->next)
    if (name && a->name == name) return a;
  return nullptr;
}
const char* TiXmlElement::Attribute(const char* name) const {
  TiXmlAttr* a = Find(name);
  return a ? a->value.c_str() : nullptr;
}
const char* TiXmlElement::Attribute(const char* name, int* i) const {
  const char* v = Attribute(name);
  if (i) *i = v ? atoi(v) : 0;
  return v;
}
const char* TiXmlElement::Attribute(const char* name, double* d) const {
  const char* v = Attribute(name);
  if (d) *d = v ? atof(v) : 0;
  return v;
}
int TiXmlElement::QueryIntAttribute(const char* name, int* value) const {
  const char* v = Attribute(name);
  if (!v) return TIXML_NO_ATTRIBUTE;
  int x;
  if (sscanf(v, "%d", &x) != 1) return TIXML_WRONG_TYPE;
  *value = x;
  return TIXML_SUCCESS;
}
int TiXmlElement::QueryDoubleAttribute(const char* name, double* value) const {
  const char* v = Attribute(name);
  if (!v) return TIXML_NO_ATTRIBUTE;
  double x;
  if (sscanf(v, "%lf", &x) != 1) return TIXML_WRONG_TYPE;
  *value = x;
  return TIXML_SUCCESS;
}
void TiXmlElement::SetAttribute(const char* name, const char* value) {
  if (!name) return;
  TiXmlAttr* a = Find(name);
  if (!a) {
    a = new TiXmlAttr;
    a->name = name;
    TiXmlAttr** end = &attrs_;
    while (*end) end = &(*end)->next;
    *end = a;
  }
  a->value = value ? value : "";
}
void TiXmlElement::SetAttribute(const char* name, int value) {
  char b[16];
  snprintf(b, sizeof b, "%d", value);
  SetAttribute(name, b);
}
void TiXmlElement::SetDoubleAttribute(const char* name, double value) {
  char b[40];
  snprintf(b, sizeof b, "%g", value);
  SetAttribute(name, b);
}
void TiXmlElement::RemoveAttribute(const char* name) {
  for (TiXmlAttr** p = &attrs_; *p; p = &(*p)->next)
    if ((*p)->name == name) {
      TiXmlAttr* a = *p;
      *p = a->next;
      delete a;
      return;
    }
}
const char* TiXmlElement::GetText() const {
  const TiXmlNode* c = FirstChild();
  return c && c->ToText() ? c->Value() : nullptr;
}
TiXmlNode* TiXmlElement::Clone() const {
  TiXmlElement* e = new TiXmlElement(value_);
  for (const TiXmlAttr* a = attrs_; a; a = a->next) e->SetAttribute(a->name.c_str(), a->value.c_str());
  CopyChildren(e);
  return e;
}
bool TiXmlElement::Accept(TiXmlPrinter* p) const {
  p->Print(this, 0);
  return true;
}

TiXmlNode* TiXmlText::Clone() const {
  TiXmlText* t = new TiXmlText(value_);
  t->cdata_ = cdata_;
  return t;
}
bool TiXmlText::Accept(TiXmlPrinter* p) const {
  p->Print(this, 0);
  return true;
}

TiXmlDeclaration::TiXmlDeclaration(const char* v, const char* e, const char* s)
    : TiXmlNode(TINYXML_DECLARATION), version_(v ? v : ""), encoding_(e ? e : ""), standalone_(s ? s : "") {}
TiXmlNode* TiXmlDeclaration::Clone() const {
  return new TiXmlDeclaration(version_.c_str(), encoding_.c_str(), standalone_.c_str());
}
bool TiXmlDeclaration::Accept(TiXmlPrinter* p) const {
  p->Print(this, 0);
  return true;
}

// ---- documents

const char* TiXmlDocument::Parse(const char* text, void*, TiXmlEncoding) {
  Clear();
  ClearError();
  size_t len = text ? strlen(text) : 0;
  oxp_code code = (oxp_code)0;
  char msg[160] = "";
  unsigned long line = 0, column = 0;
  oxp_doc* d = len ? oxp_dom_parse(text, len, OXP_OPT_NO_NAMESPACES, &code, msg, sizeof msg, &line, &column)
                   : nullptr;
  if (!d) {
    error_ = true;
    errorId_ = len ? (int)code : 12;
    errorRow_ = (int)line;
    errorCol_ = (int)column;
    errorDesc_ = len ? (msg[0] ? msg : "Error parsing XML") : "Document empty.";
    return nullptr;
  }
  Take(oxp_dom_document(d), this);
  oxp_dom_free(d);
  if (!RootElement()) {
    error_ = true;
    errorDesc_ = "Document empty.";
    return nullptr;
  }
  return text + len;
}

bool TiXmlDocument::LoadFile(const char* path, TiXmlEncoding enc) {
  if (path) SetValue(path);
  FILE* f = path ? fopen(path, "rb") : nullptr;
  if (!f) {
    Clear();
    ClearError();
    error_ = true;
    errorId_ = 2;
    errorDesc_ = "Failed to open file";
    return false;
  }
  std::string data;
  char buf[8192];
  size_t got;
  while ((got = fread(buf, 1, sizeof buf, f)) > 0) data.append(buf, got);
  fclose(f);
  return Parse(data.c_str(), nullptr, enc) != nullptr;
}

bool TiXmlDocument::SaveFile(const char* path) const {
  TiXmlPrinter p;
  Accept(&p);
  FILE* f = path ? fopen(path, "wb") : nullptr;
  if (!f) return false;
  bool ok = fwrite(p.CStr(), 1, p.Size(), f) == p.Size();
  return (fclose(f) == 0) && ok;
}

TiXmlNode* TiXmlDocument::Clone() const {
  TiXmlDocument* d = new TiXmlDocument(value_.c_str());
  CopyChildren(d);
  return d;
}
bool TiXmlDocument::Accept(TiXmlPrinter* p) const {
  for (const TiXmlNode* c = FirstChild(); c; c = c->NextSibling()) p->Print(c, 0);
  return true;
}

// ---- the printer, laid out as TinyXML's

void TiXmlPrinter::DoIndent(int depth) {
  while (depth-- > 0) buffer_ += indent_;
}

void TiXmlPrinter::Print(const TiXmlNode* n, int depth) {
  if (const TiXmlText* t = n->ToText()) {
    // text alone in its element goes inline (Print of the element did it)
    DoIndent(depth);
    if (t->CDATA()) {
      buffer_ += "<![CDATA[";
      buffer_ += t->ValueStr();
      buffer_ += "]]>";
    } else
      Escape(buffer_, t->ValueStr());
    buffer_ += lineBreak_;
    return;
  }
  if (n->Type() == TiXmlNode::TINYXML_DECLARATION) {
    const TiXmlDeclaration* d = (const TiXmlDeclaration*)n;
    DoIndent(depth);
    buffer_ += "<?xml ";
    if (*d->Version()) buffer_ += std::string("version=\"") + d->Version() + "\" ";
    if (*d->Encoding()) buffer_ += std::string("encoding=\"") + d->Encoding() + "\" ";
    if (*d->Standalone()) buffer_ += std::string("standalone=\"") + d->Standalone() + "\" ";
    buffer_ += "?>";
    buffer_ += lineBreak_;
    return;
  }
  const TiXmlElement* e = n->ToElement();
  if (!e) return;
  DoIndent(depth);
  buffer_ += '<';
  buffer_ += e->ValueStr();
  for (const TiXmlAttr* a = e->FirstAttribute(); a; a = a->next) {
    buffer_ += ' ';
    buffer_ += a->name;
    buffer_ += "=\"";
    Escape(buffer_, a->value);
    buffer_ += '"';
  }
  const TiXmlNode* c = e->FirstChild();
  if (!c) {
    buffer_ += " />";
    buffer_ += lineBreak_;
    return;
  }
  buffer_ += '>';
  if (c->ToText() && !c->NextSibling()) {
    const TiXmlText* t = c->ToText();
    if (t->CDATA()) {
      buffer_ += "<![CDATA[";
      buffer_ += t->ValueStr();
      buffer_ += "]]>";
    } else
      Escape(buffer_, t->ValueStr());
  } else {
    buffer_ += lineBreak_;
    for (; c; c = c->NextSibling()) Print(c, depth + 1);
    DoIndent(depth);
  }
  buffer_ += "</";
  buffer_ += e->ValueStr();
  buffer_ += '>';
  buffer_ += lineBreak_;
}

// ---- handles

TiXmlHandle TiXmlHandle::Child(int index) const {
  TiXmlNode* c = node_ ? node_->FirstChild() : nullptr;
  for (; c && index > 0; index--) c = c->NextSibling();
  return TiXmlHandle(c);
}
TiXmlHandle TiXmlHandle::Child(const char* value, int index) const {
  TiXmlNode* c = node_ ? node_->FirstChild(value) : nullptr;
  for (; c && index > 0; index--) c = c->NextSibling(value);
  return TiXmlHandle(c);
}
TiXmlHandle TiXmlHandle::ChildElement(int index) const {
  TiXmlElement* c = node_ ? node_->FirstChildElement() : nullptr;
  for (; c && index > 0; index--) c = c->NextSiblingElement();
  return TiXmlHandle(c);
}
TiXmlHandle TiXmlHandle::ChildElement(const char* value, int index) const {
  TiXmlElement* c = node_ ? node_->FirstChildElement(value) : nullptr;
  for (; c && index > 0; index--) c = c->NextSiblingElement(value);
  return TiXmlHandle(c);
}
