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
#include <string>

#if __has_include(<QString>)
#define LXML_QSTRING
#include <QString>
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

// RAII wrapper
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

#if     defined(LXML_QSTRING)
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

// Wtapper for easy access xmlNode
class XmlNode : public xmlNode {
    public:
        XmlNode() = delete;
        XmlNode(const XmlNode &other) = delete;
        ~XmlNode() = default;

        bool hasProp(const char *name) const {
            return xmlHasProp(this, BAD_CAST name);
        }
        std::string property(const char *name) const {
            auto txt = xmlGetProp(this, BAD_CAST name);
            if (!txt) {
                return std::string();
            }
            std::string s((char*) txt);
            xmlFree(txt);
            return s;
        }

        std::string content() const {
            auto txt = xmlNodeGetContent(this);
            if (!txt) {
                return std::string();
            }
            std::string s((char*) txt);
            xmlFree(txt);
            return s;
        }
        std::string_view name() const {
            return (char*) xmlNode::name;
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
        std::optional<std::string> toString() const {
            if (isString()) {
                return (char*) ptr->stringval;
            }
            return std::nullopt;
        }
        std::optional<std::string_view> toStringView() const {
            if (isString()) {
                return (char*) ptr->stringval;
            }
            return std::nullopt;
        }

        operator bool() const noexcept {
            return ptr;
        }
    private:
        xmlXPathObjectPtr ptr = nullptr;
};
class XPathContext {
    public:
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

inline std::string GetLastErrorString() {
    return (char*) xmlGetLastError()->message;
}

}