#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include<unordered_map>
#include<unordered_set>
#include<string>
#include<regex>
#include<errno.h>

#include "../buffer/buffer.h"


class HttpRequest{
public:
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH
    };

    enum HTTPCODE {
        NO_REQUEST = 0,
        GET_REQEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };

    HttpRequest() { Init(); };
    ~HttpRequest() = default;

    void Init();
    bool parse(Buffer &buffer);
    std::string path()const {};
    std::string& path();
    std::string method() const;
    std::string& method();
    std::string GetPost(const std::string & key) const;
    std::string GetPost(const char *key) const;

    bool IsKeepAlive() const;

private:
    bool ParseRequestLine_(const std::string &line);
    void ParseHeader_(const std::string &line);
    void ParseBody_(const std::string &line);

    void ParsePath_();
    void ParsePost_();
    void ParseFromUrlencoded_();

    static bool UserVerify(const std::string &name, const std::string &pwd, bool isLogin);

    PARSE_STATE state_;
    std::string method_, path_, version_, body_;
    std::unordered_map<std::string, std::string> header_;
    std::unordered_map<std::string, std::string> post_;
    

    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
    static int convertHex(char ch);

};

#endif