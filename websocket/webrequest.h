#pragma once

#include "buffer.h"

class WebRequest {
public:
    WebRequest()
            : method(kInvalid),
              version(kUnknown) {

    }

    enum WebSocketType {
        ERROR_FRAME = 0xff,
        CONTINUATION_FRAME = 0x00,
        TEXT_FRAME = 0x01,
        BINARY_FRAME = 0x02,
        CLOSE_FRAME = 0x08,
        PING_FRAME = 0x09,
        PONG_FRAME = 0x0A
    };

    enum Method {
        kInvalid, kGet, kPost, kHead, kPut, kDelete, kContent
    };

    enum Version {
        kUnknown, kHttp10, kHttp11
    };

    void setVersion(Version v) { version = v; }

    Version getVersion() const { return version; }

    void setMethod() { method = kContent; }

    bool setMethod(const char *start, const char *end) {
        assert(method == kInvalid);
        std::string m(start, end);
        if (m == "GET") {
            method = kGet;
        } else if (m == "POST") {
            method = kPost;
        } else if (m == "HEAD") {
            method = kHead;
        } else if (m == "PUT") {
            method = kPut;
        } else if (m == "DELETE") {
            method = kDelete;
        } else {
            method = kInvalid;
        }
        return method != kInvalid;
    }

    Method getMethod() const { return method; }

    const char *methodString() const {
        const char *result = "UNKNOWN";
        switch (method) {
            case kGet: {
                result = "GET";
                break;
            }
            case kPost: {
                result = "POST";
                break;
            }
            case kHead: {
                result = "HEAD";
                break;
            }
            case kPut: {
                result = "PUT";
                break;
            }
            case kDelete: {
                result = "DELETE";
                break;
            }
            default:
                break;

        }

        return result;
    }

    const std::string &getPath() const { return path; }

    void setPath(const char *start, const char *end) { path.assign(start, end); }

    void setQuery(const char *start, const char *end) { query.assign(start, end); }

    const std::string &getQuery() const { return query; }

    void addContent(const char *start, const char *colon, const char *end) {
        std::string field(start, colon);
        ++colon;
        while (colon < end && isspace(*colon)) {
            ++colon;
        }

        std::string value(colon, end);
        while (!value.empty() && isspace(value[value.size() - 1])) {
            value.resize(value.size() - 1);
        }

        contentLength = atoi(value.c_str());
    }

    void addHeader(const char *start, const char *colon, const char *end) {
        std::string field(start, colon);
        ++colon;
        while (colon < end && isspace(*colon)) {
            ++colon;
        }

        std::string value(colon, end);
        while (!value.empty() && isspace(value[value.size() - 1])) {
            value.resize(value.size() - 1);
        }

        headers[field] = value;
    }

    std::string getHeader(const std::string &field) const {
        std::string result;
        auto it = headers.find(field);
        if (it != headers.end()) {
            result = it->second;
        }

        return result;
    }

    const std::map <std::string, std::string> &getHeaders() const { return headers; }

    void reset() {
        method = kInvalid;
        version = kUnknown;
        path.clear();
        query.clear();
        queryLength = 0;
        contentLength = 0;
        headers.clear();
        parseString.clear();
        setOpCode();
    }

    WebSocketType &getOpCode() { return opcode; }

    void setOpCode() { opcode = ERROR_FRAME; }

    void setOpCodeType(WebSocketType op) { opcode = op; }

    std::string &getCacheFrame() { return cacheFrame; }

    std::string &getParseString() { return parseString; }

private:
    Method method;
    Version version;
    std::string path;
    std::string query;
    int32_t queryLength;
    int32_t contentLength;
    WebSocketType opcode;
    std::map <std::string, std::string> headers;
    std::string cacheFrame;
    std::string parseString;
};
