/* The part of TinyXML that OpenCPN uses (its style files, styles.cpp, and
 * KML on the clipboard, kml.cpp), with OXP, GFOpenXML's own parser, reading
 * the text: OXP parses, and the tree it gives is taken into TinyXML's own
 * shape of tree (nodes made with new and handed to a parent, which then owns
 * them), so OpenCPN's code is unchanged. This header takes the place of
 * libs/tinyxml/include/tinyxml.h for this port; src/tinyxml_oxp.cpp holds
 * the code.
 *
 * As TinyXML's defaults: white space in text is condensed (runs to one
 * space, none at either end) and text that is only white space is dropped.
 * Comments and processing instructions are not kept. Names are taken whole,
 * prefixes and all.
 * Copyright (c) 2026 AmiCradle Team (GPL v2 or later, as OpenCPN) */
#ifndef OCPN_TINYXML_OXP_H
#define OCPN_TINYXML_OXP_H

#include <string>

enum TiXmlEncoding { TIXML_ENCODING_UNKNOWN, TIXML_ENCODING_UTF8, TIXML_ENCODING_LEGACY };
const TiXmlEncoding TIXML_DEFAULT_ENCODING = TIXML_ENCODING_UNKNOWN;
enum { TIXML_SUCCESS, TIXML_NO_ATTRIBUTE, TIXML_WRONG_TYPE };

class TiXmlDocument;
class TiXmlElement;
class TiXmlText;
class TiXmlDeclaration;
class TiXmlPrinter;
struct TiXmlAttr;

class TiXmlNode {
public:
  enum NodeType {
    TINYXML_DOCUMENT,
    TINYXML_ELEMENT,
    TINYXML_COMMENT,
    TINYXML_UNKNOWN,
    TINYXML_TEXT,
    TINYXML_DECLARATION,
    TINYXML_TYPECOUNT
  };
  virtual ~TiXmlNode();

  int Type() const { return type_; }
  const char* Value() const { return value_.c_str(); }
  const std::string& ValueStr() const { return value_; }
  void SetValue(const char* v) { value_ = v ? v : ""; }
  void SetValue(const std::string& v) { value_ = v; }

  TiXmlNode* Parent() { return parent_; }
  const TiXmlNode* Parent() const { return parent_; }
  TiXmlNode* FirstChild() { return first_; }
  const TiXmlNode* FirstChild() const { return first_; }
  TiXmlNode* FirstChild(const char* value);
  const TiXmlNode* FirstChild(const char* value) const;
  TiXmlNode* LastChild() { return last_; }
  TiXmlNode* NextSibling() { return next_; }
  const TiXmlNode* NextSibling() const { return next_; }
  TiXmlNode* NextSibling(const char* value);
  TiXmlNode* PreviousSibling() { return prev_; }
  TiXmlElement* FirstChildElement();
  const TiXmlElement* FirstChildElement() const;
  TiXmlElement* FirstChildElement(const char* value);
  const TiXmlElement* FirstChildElement(const char* value) const;
  TiXmlElement* NextSiblingElement();
  const TiXmlElement* NextSiblingElement() const;
  TiXmlElement* NextSiblingElement(const char* value);
  const TiXmlElement* NextSiblingElement(const char* value) const;
  bool NoChildren() const { return !first_; }

  TiXmlNode* LinkEndChild(TiXmlNode* add);       // takes it: the parent owns it
  TiXmlNode* InsertEndChild(const TiXmlNode& add);  // a copy
  bool RemoveChild(TiXmlNode* remove);
  void Clear();

  virtual TiXmlNode* Clone() const = 0;
  virtual bool Accept(TiXmlPrinter* visitor) const = 0;

  TiXmlDocument* ToDocument() { return type_ == TINYXML_DOCUMENT ? (TiXmlDocument*)this : nullptr; }
  TiXmlElement* ToElement() { return type_ == TINYXML_ELEMENT ? (TiXmlElement*)this : nullptr; }
  const TiXmlElement* ToElement() const {
    return type_ == TINYXML_ELEMENT ? (const TiXmlElement*)this : nullptr;
  }
  TiXmlText* ToText() { return type_ == TINYXML_TEXT ? (TiXmlText*)this : nullptr; }
  const TiXmlText* ToText() const { return type_ == TINYXML_TEXT ? (const TiXmlText*)this : nullptr; }
  TiXmlDeclaration* ToDeclaration() {
    return type_ == TINYXML_DECLARATION ? (TiXmlDeclaration*)this : nullptr;
  }

protected:
  explicit TiXmlNode(NodeType t) : type_(t) {}
  void CopyChildren(TiXmlNode* to) const;
  NodeType type_;
  std::string value_;
  TiXmlNode *parent_ = nullptr, *first_ = nullptr, *last_ = nullptr, *prev_ = nullptr, *next_ = nullptr;

private:
  TiXmlNode(const TiXmlNode&) = delete;
  TiXmlNode& operator=(const TiXmlNode&) = delete;
};

struct TiXmlAttr {
  std::string name, value;
  TiXmlAttr* next = nullptr;
};

class TiXmlElement : public TiXmlNode {
public:
  explicit TiXmlElement(const char* value);
  explicit TiXmlElement(const std::string& value);
  ~TiXmlElement() override;

  const char* Attribute(const char* name) const;
  const char* Attribute(const char* name, int* i) const;
  const char* Attribute(const char* name, double* d) const;
  int QueryIntAttribute(const char* name, int* value) const;
  int QueryDoubleAttribute(const char* name, double* value) const;
  void SetAttribute(const char* name, const char* value);
  void SetAttribute(const char* name, int value);
  void SetAttribute(const std::string& name, const std::string& value) {
    SetAttribute(name.c_str(), value.c_str());
  }
  void SetDoubleAttribute(const char* name, double value);
  void RemoveAttribute(const char* name);
  const TiXmlAttr* FirstAttribute() const { return attrs_; }

  // the text of its first child, when that is text
  const char* GetText() const;

  TiXmlNode* Clone() const override;
  bool Accept(TiXmlPrinter* visitor) const override;

private:
  TiXmlAttr* Find(const char* name) const;
  TiXmlAttr* attrs_ = nullptr;
};

class TiXmlText : public TiXmlNode {
public:
  explicit TiXmlText(const char* value) : TiXmlNode(TINYXML_TEXT) { SetValue(value); }
  explicit TiXmlText(const std::string& value) : TiXmlNode(TINYXML_TEXT) { SetValue(value); }
  bool CDATA() const { return cdata_; }
  void SetCDATA(bool c) { cdata_ = c; }
  TiXmlNode* Clone() const override;
  bool Accept(TiXmlPrinter* visitor) const override;

private:
  bool cdata_ = false;
};

class TiXmlDeclaration : public TiXmlNode {
public:
  TiXmlDeclaration(const char* version, const char* encoding, const char* standalone);
  const char* Version() const { return version_.c_str(); }
  const char* Encoding() const { return encoding_.c_str(); }
  const char* Standalone() const { return standalone_.c_str(); }
  TiXmlNode* Clone() const override;
  bool Accept(TiXmlPrinter* visitor) const override;

private:
  std::string version_, encoding_, standalone_;
};

class TiXmlDocument : public TiXmlNode {
public:
  TiXmlDocument() : TiXmlNode(TINYXML_DOCUMENT) {}
  explicit TiXmlDocument(const char* name) : TiXmlNode(TINYXML_DOCUMENT) { SetValue(name); }

  // the end of what was read, or NULL on an error (Error and ErrorDesc say why)
  const char* Parse(const char* text, void* data = nullptr, TiXmlEncoding enc = TIXML_DEFAULT_ENCODING);
  bool LoadFile(TiXmlEncoding enc = TIXML_DEFAULT_ENCODING) { return LoadFile(Value(), enc); }
  bool LoadFile(const char* path, TiXmlEncoding enc = TIXML_DEFAULT_ENCODING);
  bool LoadFile(const std::string& path, TiXmlEncoding enc = TIXML_DEFAULT_ENCODING) {
    return LoadFile(path.c_str(), enc);
  }
  bool SaveFile(const char* path) const;
  bool SaveFile() const { return SaveFile(Value()); }

  TiXmlElement* RootElement() { return FirstChildElement(); }
  const TiXmlElement* RootElement() const { return FirstChildElement(); }
  bool Error() const { return error_; }
  const char* ErrorDesc() const { return errorDesc_.c_str(); }
  int ErrorId() const { return errorId_; }
  int ErrorRow() const { return errorRow_; }
  int ErrorCol() const { return errorCol_; }
  void ClearError() {
    error_ = false;
    errorId_ = errorRow_ = errorCol_ = 0;
    errorDesc_.clear();
  }

  TiXmlNode* Clone() const override;
  bool Accept(TiXmlPrinter* visitor) const override;

private:
  bool error_ = false;
  int errorId_ = 0, errorRow_ = 0, errorCol_ = 0;
  std::string errorDesc_;
};

// TinyXML's printer: the tree as text, indented
class TiXmlPrinter {
public:
  void SetIndent(const char* i) { indent_ = i ? i : ""; }
  const char* Indent() { return indent_.c_str(); }
  void SetLineBreak(const char* b) { lineBreak_ = b ? b : ""; }
  void SetStreamPrinting() {
    indent_.clear();
    lineBreak_.clear();
  }
  const char* CStr() { return buffer_.c_str(); }
  size_t Size() { return buffer_.size(); }
  const std::string& Str() { return buffer_; }

  void Print(const TiXmlNode* n, int depth);  // used by Accept

private:
  void DoIndent(int depth);
  std::string buffer_, indent_ = "    ", lineBreak_ = "\n";
};

// TinyXML's handle: a pointer that may be NULL, so lookups can be chained
class TiXmlHandle {
public:
  TiXmlHandle(TiXmlNode* node) : node_(node) {}
  TiXmlHandle FirstChild() const { return TiXmlHandle(node_ ? node_->FirstChild() : nullptr); }
  TiXmlHandle FirstChild(const char* value) const {
    return TiXmlHandle(node_ ? node_->FirstChild(value) : nullptr);
  }
  TiXmlHandle FirstChildElement() const {
    return TiXmlHandle(node_ ? (TiXmlNode*)node_->FirstChildElement() : nullptr);
  }
  TiXmlHandle FirstChildElement(const char* value) const {
    return TiXmlHandle(node_ ? (TiXmlNode*)node_->FirstChildElement(value) : nullptr);
  }
  TiXmlHandle Child(int index) const;
  TiXmlHandle Child(const char* value, int index) const;
  TiXmlHandle ChildElement(int index) const;
  TiXmlHandle ChildElement(const char* value, int index) const;
  TiXmlNode* ToNode() const { return node_; }
  TiXmlElement* ToElement() const { return node_ ? node_->ToElement() : nullptr; }
  TiXmlText* ToText() const { return node_ ? node_->ToText() : nullptr; }
  TiXmlNode* Node() const { return ToNode(); }
  TiXmlElement* Element() const { return ToElement(); }
  TiXmlText* Text() const { return ToText(); }

private:
  TiXmlNode* node_;
};

#endif
