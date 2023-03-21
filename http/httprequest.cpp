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

