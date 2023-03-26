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
    LOG_DEBUG("Connection is [%s], version_ is [%s]", header_.find("Connection")->second.c_str(), version_.c_str());
    if (header_.count("Connection")) {
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
        // std::cout<<line<<std::endl;
        // LOG_INFO("Parsing line: %s, state is header ? [%d]", line.data(), state_==HEADERS);
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
    LOG_INFO("Parsing done, request path is : %s", path_.data());
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
    std::regex pattern("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if(std::regex_match(line, subMatch, pattern)) {
        header_[subMatch[1]] = subMatch[2];
        LOG_DEBUG("[%s] is [%s]",std::string(subMatch[1]).c_str(), std::string(subMatch[2]).c_str() );
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
    if (name == "" || pwd == "") { return false;}
    LOG_INFO("Verifying name:%s pwd:%s", name.c_str(), pwd.c_str());
    MYSQL *sql = nullptr;
    SqlConnRAII(&sql, SqlConnPool::Instance()); // TODO("第二个参数是否会出错当场被释放")
    assert(sql);

    bool flag = false;
    unsigned int j = 0;
    char order[256] = {0};

    MYSQL_FIELD *fields = nullptr;
    MYSQL_RES *res = nullptr;

    if (!isLogin) {flag = true;}
    snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", name.c_str());

    LOG_DEBUG("Executing quering : %s", order);
    if (mysql_query(sql, order)) {
        LOG_ERROR("Quering failed!");
        return false;
    }

    res = mysql_store_result(sql);
    j = mysql_num_fields(res);
    fields = mysql_fetch_field(res);

    while(MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG("MYSQL_ROW: %s %s", row[0], row[1]);
        std::string password(row[1]);
        if (isLogin) {
            if (pwd == password) { flag = true;}
            else {
                flag = false;
                LOG_DEBUG("Invalid password!");
            }
        } else {
            flag = false;
            LOG_DEBUG("User %s is Already exist!");
        }
    }
    mysql_free_result(res);

    if (!isLogin && flag) {
        LOG_DEBUG("regirsting!");
        bzero(order, 256);
        snprintf(order, 256, "INSERT INTO user(username, passord) VALUES('%s',  '%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG("%s", order);
        if (mysql_query(sql, order)) {
            LOG_DEBUG("Insert Failed!");
            flag = false;
        }
    }
    SqlConnPool::Instance()->FreeConn(sql);
    LOG_DEBUG("UserVerify results is %d", flag);
    return flag;
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
