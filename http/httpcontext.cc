#include "httpcontext.h"
#include "buffer.h"

bool HttpContext::processResponseLine(const char *begin, const char *end) {
    bool succeed = false;
    const char *start = begin;
    const char *space = std::find(start, end, ' ');
    if (space != end) {
        succeed = space - start == 8 && std::equal(start, space - 1, "HTTP/1.");
        if (succeed) {
            if (*(space - 1) == '1') {

            } else if (*(space - 1) == '0') {

            } else {
                succeed = false;
            }
        }

        start = space + 1;
        space = std::find(start, end, ' ');
        if (space != end) {
            response.setStatusCode(start, space);
            start = space + 1;
            if (start != end) {
                response.setStatusMessage(start, end);
            }
        }
    }
    return succeed;
}

bool HttpContext::processRequestLine(const char *begin, const char *end) {
    bool succeed = false;
    const char *start = begin;
    const char *space = std::find(start, end, ' ');
    if (space != end && request.setMethod(start, space)) {
        start = space + 1;
        space = std::find(start, end, ' ');
        if (space != end) {
            const char *question = std::find(start, space, '?');
            if (question != space) {
                request.setPath(start, question);
                request.setQuery(question + 1, space);
            } else {
                request.setPath(start, space);
            }

            start = space + 1;
            succeed = end - start == 8 && std::equal(start, end - 1, "HTTP/1.");
            if (succeed) {
                if (*(end - 1) == '1') {
                    request.setVersion(HttpRequest::kHttp11);
                } else if (*(end - 1) == '0') {
                    request.setVersion(HttpRequest::kHttp10);
                } else {
                    succeed = false;
                }
            }
        }
    }
    return succeed;
}

bool HttpContext::parseResponse(Buffer *buf) {
    bool ok = true;
    bool hasMore = true;
    while (hasMore) {
        if (state == kExpectRequestLine) {
            const char *crlf = buf->findCRLF();
            if (crlf) {
                ok = processResponseLine(buf->peek(), crlf);
                if (ok) {
                    buf->retrieveUntil(crlf + 2);
                    state = kExpectHeaders;
                } else {
                    hasMore = false;
                }
            } else {
                hasMore = false;
            }
        } else if (state == kExpectHeaders) {
            const char *crlf = buf->findCRLF();
            if (crlf) {
                const char *colon = std::find(buf->peek(), crlf, ':');
                if (colon != crlf) {
                    response.addHeader(buf->peek(), colon, crlf);
                } else {
                    state = kExpectBody;
                }
                buf->retrieveUntil(crlf + 2);
            } else {
                hasMore = false;
            }
        } else if (state == kExpectBody) {
            hasMore = false;
            int32_t size = response.getBodySize();
            if (buf->readableBytes() == size) {
                response.setBody(std::string(buf->peek(), buf->readableBytes()));
                buf->retrieveAll();
                state = kGotAll;
            }
        }
    }
    return ok;
}

bool HttpContext::parseRequest(Buffer *buf) {
    bool ok = true;
    bool hasMore = true;
    while (hasMore) {
        if (state == kExpectRequestLine) {
            const char *crlf = buf->findCRLF();
            if (crlf) {
                ok = processRequestLine(buf->peek(), crlf);
                if (ok) {
                    buf->retrieveUntil(crlf + 2);
                    state = kExpectHeaders;
                } else {
                    hasMore = false;
                }
            } else {
                hasMore = false;
            }
        } else if (state == kExpectHeaders) {
            const char *crlf = buf->findCRLF();
            if (crlf) {
                const char *colon = std::find(buf->peek(), crlf, ':');
                if (colon != crlf) {
                    request.addHeader(buf->peek(), colon, crlf);
                } else {
                    state = kGotAll;
                    hasMore = false;
                }
                buf->retrieveUntil(crlf + 2);
            } else {
                hasMore = false;
            }
        } else if (state == kExpectBody) {

        }
    }
    return ok;
}




























