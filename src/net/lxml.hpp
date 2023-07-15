/**
 * @file lxml.hpp
 * @author BusyStudent (fyw90mc@gmali.com)
 * @brief A Tiny libxml2 wrapper for crawling html data by xpath
 * @version 0.1
 * @date 2023-06-16
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once

#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <optional>

#if __has_include(<QString>)
    #define LXML_HAS_QSTRING
    #define LXML_STRING         QString
    #define LXML_STRING_FROM(x) QString::fromUtf8(x)
    #define LXML_STRINGLIST     QStringList
    #define LXML_DECL_QDEBUG(x) friend QDebug operator <<(QDebug, const x &)
    #include <QString>
    #include <QDebug>
#else
    #include <string>
    #include <vector>
    #define LXML_STRING         std::string
    #define LXML_STRING_FROM(x) std::string((char*) x)
    #define LXML_STRINGLIST     std::vector<std::string>
    #define LXML_DECL_QDEBUG(x) 
#endif

namespace LXml {

inline constexpr int HtmlOptions = 
    HTML_PARSE_RECOVER | 
    HTML_PARSE_NOERROR | 
    HTML_PARSE_NOWARNING | 
    HTML_PARSE_NONET | 
    HTML_PARSE_NOBLANKS
;
inline constexpr int XmlOptions = 
    XML_PARSE_RECOVER |
    XML_PARSE_NOERROR |
    XML_PARSE_NOWARNING |
    XML_PARSE_NONET |
    XML_PARSE_NOBLANKS
;

using String = LXML_STRING;
using StringList = LXML_STRINGLIST;
template <typename T>
using Ptr = std::unique_ptr<T>;

template <typename T>
inline String CreateString(const T *ch) {
    if (ch) {
        return LXML_STRING_FROM(ch);
    }
    return String();
}

template <typename T>
class MemPtr {
    public:
        MemPtr() = default;
        MemPtr(T *value) noexcept : value(value) { }
        MemPtr(const MemPtr &) = delete;
        MemPtr(MemPtr &&m) noexcept {
            value = m.release();
        }
        ~MemPtr() noexcept {
            reset();
        }

        void reset(T *nValue = nullptr) noexcept {
            if (value) {
                ::xmlFree(value);
            }
            value = nValue;
        }
        T  *release(T *nValue = nullptr) noexcept {
            T  *prev = value;
            value = nValue;
            return prev;
        }
        T  *get() const noexcept {
            return value;
        }

        MemPtr &operator =(MemPtr &&other) noexcept {
            reset(other.release());
            return *this;
        }

        operator bool() const noexcept {
            return value;
        }
    private:
        T *value = nullptr;
};

// RAII wrapper
class XPathObject;
class HtmlDocoument {
    public:
        HtmlDocoument() = default;
        HtmlDocoument(htmlDocPtr p) : doc(p) { }
        HtmlDocoument(HtmlDocoument && d) {
            doc = d.release();
        }
        ~HtmlDocoument() {
            reset();
        }

        void reset(htmlDocPtr nptr = nullptr) {
            if (doc) {
                xmlFreeDoc(doc);
            }
            doc = nptr;
        }
        htmlDocPtr release(htmlDocPtr nptr = nullptr) {
            htmlDocPtr tmp = doc; 
            doc = nptr; 
            return tmp; 
        }
        htmlDocPtr get() const noexcept {
            return doc;
        }
        htmlDocPtr operator ->() const noexcept {
            return doc;
        }
        XPathObject xpath(const char *exp) const;

        HtmlDocoument &operator =(HtmlDocoument &&d) {
            reset(d.release());
            return *this;
        }

        operator bool() const noexcept {
            return doc;
        }
        

        static HtmlDocoument Parse(const std::string &v, int options = HtmlOptions) {
            return htmlReadDoc(
                BAD_CAST v.c_str(),
                nullptr, "utf8", 
                options
            );
        }

#if     defined(LXML_HAS_QSTRING)
        static HtmlDocoument Parse(const QByteArray &v, int options = HtmlOptions) {
            return htmlReadMemory(
                v.constData(),
                v.size(),
                nullptr, "utf8", 
                options
            );
        }
#endif
    private:
        htmlDocPtr doc = nullptr;
};

template <typename T>
class XmlIterator {
    public:
        XmlIterator(T *value = nullptr) : value(value) { }
        XmlIterator(const XmlIterator &) = default;
        ~XmlIterator() = default;

        XmlIterator &operator ++() noexcept {
            value = value->next;
            return *this;
        }
        XmlIterator &operator --() noexcept {
            value = value->prev;
            return *this;
        }
        XmlIterator  operator ++(int) noexcept {
            auto i = *this;
            (*this) ++;
            return i;
        }
        XmlIterator  operator --(int) noexcept {
            auto i = *this;
            (*this) --;
            return i;
        }
        T           &operator *() const noexcept {
            return *value;
        }
        T           *operator ->() const noexcept {
            return value;
        }
        bool         operator ==(const XmlIterator &other) const noexcept {
            return value == other.value;
        }
        bool         operator !=(const XmlIterator &other) const noexcept {
            return value == other.value;
        }
    private:
        T *value;
};

// Wrapper for easy access xmlAttr
class XmlAttr : public xmlAttr {
    public:
        XmlAttr() = delete;
        XmlAttr(const XmlAttr &other) = delete;
        ~XmlAttr() {
            xmlFreeProp(this);
        }

        String content() const {
            MemPtr<xmlChar> txt = xmlNodeGetContent(children);
            return CreateString(txt.get());
        }
        QString name() const {
            return CreateString(xmlAttr::name);
        }

        // Memory
        void *operator new(size_t n) = delete;
        void  operator delete(void *, size_t n) { }
};
// Wrapper for easy access xmlNode
class XmlNode : public xmlNode {
    public:
        XmlNode() = delete;
        XmlNode(const XmlNode &other) = delete;
        ~XmlNode() {
            xmlUnlinkNode(this);
            xmlFreeNode(this);
        }

        bool hasProp(const char *name) const {
            return xmlHasProp(this, BAD_CAST name);
        }
        String property(const char *name) const {
            MemPtr<xmlChar> txt = xmlGetProp(this, BAD_CAST name);
            return CreateString(txt.get());
        }

        String content() const {
            MemPtr<xmlChar> txt = xmlNodeGetContent(this);
            return CreateString(txt.get());
        }
        String name() const {
            return CreateString(xmlNode::name);
        }

        XmlNode *children() const {
            return (XmlNode*) xmlNode::children;
        }
        XmlNode *parent() const {
            return (XmlNode*) xmlNode::parent;
        }
        XmlNode  *next() const {
            return (XmlNode*) xmlNode::next;
        }
        XmlNode  *prev() const {
            return (XmlNode*) xmlNode::prev;
        }
        XmlNode  *last() const {
            return (XmlNode*) xmlNode::last;
        }
        // Depth first search
        XmlNode   *findChild(const char *name) const {
            if (xmlStrcmp(xmlNode::name, BAD_CAST name) == 0) {
                return const_cast<XmlNode*>(this);
            }
            // For children
            for (XmlNode *child = children(); child; child = child->next()) {
                auto v = child->findChild(name);
                if (v) {
                    return v;
                }
            }
            return nullptr;
        }
        XPathObject xpath(const char *exp) const;

        // Operation
        Ptr<XmlNode> unlink() {
            xmlUnlinkNode(this);
            return Ptr<XmlNode>(this);
        }
        Ptr<XmlNode> clone(bool recursive = true) {
            return FromRawPtr(xmlCopyNode(this, recursive));
        }
        bool         addChild(Ptr<XmlNode> &&x) {
            if (xmlAddChild(this, x.get())) {
                x.release();
                return true;
            }
            return false;
        }

        // Memory
        void *operator new(size_t n) = delete;
        void  operator delete(void *, size_t n) { }

        // Debug
        LXML_DECL_QDEBUG(XmlNode);

        static Ptr<XmlNode> New(const char *name) {
            return FromRawPtr(xmlNewNode(nullptr, BAD_CAST name));
        }
        static Ptr<XmlNode> FromRawPtr(xmlNodePtr ptr) noexcept {
            return Ptr<XmlNode>(static_cast<XmlNode*>(ptr));
        }
};

class XPathObject {
    public:
        XPathObject(xmlXPathObjectPtr p = nullptr) : ptr(p) { }
        XPathObject(XPathObject &&other) : ptr(other.release()) { }
        ~XPathObject() {

        }
        void reset(xmlXPathObjectPtr p = nullptr) {
            xmlXPathFreeObject(ptr);
            ptr = p;
        }
        xmlXPathObjectPtr release(xmlXPathObjectPtr p = nullptr) {
            xmlXPathObjectPtr prev = ptr;
            ptr = p;
            return prev;
        }

        XPathObject &operator =(XPathObject &&d) {
            reset(d.release());
            return *this;
        }
        XPathObject  clone() const {
            return xmlXPathObjectCopy(ptr);
        }

        bool empty() const {
            return ptr == nullptr;
        }
        bool isNodeset() const {
            return ptr && ptr->type == XPATH_NODESET;
        }
        bool isNumber() const {
            return ptr && ptr->type == XPATH_NUMBER;
        }
        bool isBool() const {
            return ptr && ptr->type == XPATH_BOOLEAN;
        }
        bool isString() const {
            return ptr && ptr->type == XPATH_STRING;
        }
        XmlNode **begin() const {
            if (!ptr->nodesetval) {
                return nullptr;
            }
            return (XmlNode **) ptr->nodesetval->nodeTab;
        }
        XmlNode **end() const {
            if (!ptr->nodesetval) {
                return nullptr;
            }
            return (XmlNode **) ptr->nodesetval->nodeTab + ptr->nodesetval->nodeNr;
        }
        XmlNode **nodeTab() const {
            if (!ptr->nodesetval) {
                return nullptr;
            }
            return (XmlNode **) ptr->nodesetval->nodeTab;
        }
        int        nodeCount() const {
            if (!ptr->nodesetval) {
                return 0;
            }
            return ptr->nodesetval->nodeNr;
        }
        std::optional<bool>   toBool() const {
            if (isBool()) {
                return ptr->boolval;
            }
            return std::nullopt;
        }
        std::optional<double> toNumber() const {
            if (isNumber()) {
                return ptr->floatval;
            }
            return std::nullopt;
        }
        std::optional<String> toString() const {
            if (isString()) {
                return  CreateString(ptr->stringval);
            }
            return std::nullopt;
        }

        operator bool() const noexcept {
            return ptr;
        }

        LXML_DECL_QDEBUG(XPathObject);
    private:
        xmlXPathObjectPtr ptr = nullptr;
};
class XPathContext {
    public:
        XPathContext(xmlDocPtr doc) {
            ctxt = xmlXPathNewContext(doc);
        }
        XPathContext(const HtmlDocoument &doc) {
            ctxt = xmlXPathNewContext(doc.get());
        }
        XPathContext(const XPathContext &&) = delete;
        ~XPathContext() {
            if (ctxt) {
                xmlXPathFreeContext(ctxt);
            }
        }

        XPathObject eval(const char *str) {
            return xmlXPathEvalExpression(BAD_CAST str, ctxt);
        }
        XPathObject eval(xmlNodePtr node, const char *str) {
            return xmlXPathNodeEval(node, BAD_CAST str, ctxt);
        }
    private:
        xmlXPathContextPtr ctxt = nullptr;
};

inline XPathObject HtmlDocoument::xpath(const char *exp) const {
    if (!get()) {
        // No document
        return XPathObject();
    }
    XPathContext ctxt(*this);
    return ctxt.eval(exp);
}
inline XPathObject XmlNode::xpath(const char *exp) const {
    XPathContext ctxt(doc);
    return ctxt.eval(const_cast<XmlNode*>(this), exp);
}
inline String GetLastErrorString() {
    return CreateString(xmlGetLastError()->message);
}

// Qt Debugging output
#if defined(LXML_HAS_QSTRING)
inline QDebug operator <<(QDebug s, const XmlNode &xmlnode) {
    s << QStringLiteral("XmlNode(%1 : %2)").arg(xmlnode.name(), xmlnode.content());
    return s;
}
inline QDebug operator <<(QDebug s, const XPathObject &object) {
    s << "XPathObject(";
    if (object.empty()) {

    }
    else if (object.isBool()) {
        s << object.toBool().value_or(false);
    }
    else if (object.isNumber()) {
        s << object.toNumber().value_or(0);
    }
    else if (object.isString()) {
        s << object.toString().value_or(QString());
    }
    else if (object.isNodeset()) {
        s << "Nodeset(";
        for (const auto node : object) {
            s << *node;
        }
        s << ")";
    }
    s << ")";
    return s;
}

#endif

}