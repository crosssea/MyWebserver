#include "httprequest.h"

const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML{
    "/index", "/register", "/login",
    "/welcome", "/videp", "/picture"
};

const std::unordered_map<std::string, int> HttpRequest::DEFAULT_HTML_TAG{
    {"/register.html", 0}, {"/login.html", 1}
};

void HttpRequest::Init() {
    method_ = path_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

bool HttpRequest::IsKeepAlive() const {
    if (header_.count("connection") == 1) {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

bool HttpRequest::parse(Buffer &buff) {
    const char CRLF[] = "\r\n";
    if (buff.ReadableBytes() <= 0) {
        return false;
    }
    while (buff.ReadableBytes() && state_ != FINISH) {
        const char *lineEnd = std::search(buff.Peek(), buff.BeginWriteConst(), CRLF, CRLF + 2); //需要时类型匹配的
        std::string line(buff.Peek(), lineEnd);
        switch (state_)
        {
        case REQUEST_LINE:
            if(!ParseRequestLine_(line)) {
                return false;
            }
            ParsePath_();
            break;
        case HEADERS:
            ParseHeader_(line);
            if (buff.ReadableBytes() <= 2) {
                state_ = FINISH;
            }
            break;
        case BODY:
            ParseBody_(line);
            break;
        default:
            break;
        }
        if (lineEnd == buff.BeginWrite()) {
            break;
        }
        buff.RetrieveUntil(lineEnd + 2);
    }
    return true;
}

void HttpRequest::ParsePath_ () {
    if (path_ == "/") {
        path_ = "/index.html";
    } else if (DEFAULT_HTML.count(path_)){
        path_ += ".html";
    }
}
/*
包含方法 请求路径 协议版本
*/
bool HttpRequest::ParseRequestLine_ (const std::string &line) {
    std::regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;
    if (std::regex_match(line, subMatch, pattern)){
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];
        state_ = HEADERS;
        return true;
    }
    return false;
}
/*
因为时一行一行的存储键值对
最后和Body会隔开一行 ～～～
*/

void HttpRequest::ParseHeader_ (const std::string &line) {
    std::regex pattern("^(^:)*: ?(.*)$");
    std::smatch subMatch;
    if(std::regex_match(line, subMatch, pattern)) {
        header_[subMatch[1]] = header_[subMatch[2]];
    } else {
        state_ = BODY;
    }
}

void HttpRequest::ParseBody_ (const std::string &line) {
    body_ = line;
    ParsePost_();
    state_ = FINISH;
}

int HttpRequest::convertHex (char ch) {
    if(ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if(ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return ch;
}

void HttpRequest::ParsePost_ () {
    if (method_ == "POST" && header_["Content-Type"] == "application/x-www-from-urlencoded") {
        ParseFromUrlencoded_();
        if (DEFAULT_HTML_TAG.count(path_)) {
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            if (tag == 0 || tag == 1) {
                if (UserVerify(post_["username"], post_["password"], tag)) {
                    path_= "/welcome.html";
                } else {
                    path_ = "/error.html";
                }
            }
        }
    }
}

void HttpRequest::ParseFromUrlencoded_ () {
    if (body_.size() == 0) {return; }

    std::string key,value;
    int num = 0;
    int n = body_.size();
    for (int i = 0, j = 0; i < n; i++) {

    }
    
}