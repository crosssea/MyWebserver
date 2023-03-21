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

int HttpRequest::ConvertHex (char ch) {
    if(ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if(ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return ch - '0';
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
    int i = 0, j = 0;
    for (; i < n; i++) {
        char ch = body_[i];
        switch (ch)
        {
        case '=':
            key = body_.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            body_[i] = ' ';
            break;
        case '%':
            num = ConvertHex(body_[i + 1]) * 16 + ConvertHex(body_[i + 2]);
            body_[i+2] = num % 10 + '0';
            body_[i+1] = num / 10 + '0';
            i += 2;
            break;
        case '&':
            value = body_.substr(j, i - j);
            j = i + 1;
            post_[key] = value;
            break;
        default:
            break;
        }
    }
    assert(j <= i);
    if (post_.count(key) == 0 && j < i) {
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
}
/*
TODO(添加数据库连接池，检测用户是否登陆等等)
*/
bool HttpRequest::UserVerify (const std::string &name, const std::string &pwd, bool isLogin) {
    return true;
}

std::string HttpRequest::path() const{
    return path_;
}

std::string& HttpRequest::path() {
    return path_;
}

std::string HttpRequest::method() const {
    return method_;
}

std::string HttpRequest::version() const {
    return version_;
}

std::string HttpRequest::GetPost(const std::string &key) const {
    assert(key != "");
    if (post_.count(key)){
        return post_.find(key)->second; // 无法消除const
    }
    return "";
}

std::string HttpRequest::GetPost(const char *key) const {
    assert(key != nullptr);
    if (post_.count(key)){
        return post_.find(key)->second; // 无法消除const
    }
    return "";
}
