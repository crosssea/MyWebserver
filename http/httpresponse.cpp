#include "./httpresponse.h"


const std::unordered_map<std::string, std::string> HttpResponse::SUFFIX_TYPE = {
    {".html", "text/html"},
    {".xml", "text/xml"},
    {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".word", "application/nsword"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".au", "audio/basic"},
    {".mpeg", "video/,peg"},
    {".mpg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},
    {".tar", "application/x-tar"},
    {".css", "text/css"},
    {".js", "text/javascript"},

};

const std::unordered_map<int, std::string> HttpResponse::CODE_STATUS = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
};

const std::unordered_map<int, std::string> HttpResponse::CODE_PATH = {
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"},
};

HttpResponse::HttpResponse() {
    code_ = -1;
    path_ = srcDir_ = "";
    mmFile_ = nullptr;
    mmFileStat_ = {0};
    isKeepAlive_ = false;
}

HttpResponse::~HttpResponse() {
    UnmapFile();
}


void HttpResponse::Init(const std::string &srcDir, const std::string &path, bool isKeepAlive, int code) {
    assert(srcDir != "");
    if (mmFile_) { UnmapFile();}
    code_ = code;
    path_ = path;
    srcDir_ = srcDir;
    isKeepAlive_ = isKeepAlive;
    mmFile_ = nullptr;
    mmFileStat_ = {0};
    LOG_INFO("Response file is: %s",(srcDir_ + path_).data());
}

void HttpResponse::MakeResponse(Buffer &buff) {
    if (stat((srcDir_ + path_).data(), &mmFileStat_) < 0 || S_ISDIR(mmFileStat_.st_mode)) {
        // std::cout<<404<<std::endl;
        code_ = 404; // 找不到要的文件，或者是一个路径
        LOG_WARN("File not found: %s", (srcDir_ + path_).data())
    }
    else if (!(mmFileStat_.st_mode & S_IROTH)) {
        code_ = 403; //没有权限
    } else if (code_ == -1) {
        code_ = 200; //成功
    }
    ErrorHtml_();
    LOG_DEBUG("After Error Handling file is [%s]",(srcDir_ + path_).data());
    AddStateLine_(buff);
    AddHeader_(buff);
    AddContent_(buff);
}

char * HttpResponse::File() {
    return mmFile_;
}

size_t HttpResponse::FileLen() const {
    return mmFileStat_.st_size;
}

void HttpResponse::ErrorHtml_ () {
    if (CODE_PATH.count(code_)) {
        path_ = CODE_PATH.find(code_)->second; // []会报错
        stat((srcDir_ + path_).data(), &mmFileStat_);
    }
}

void HttpResponse::AddStateLine_ (Buffer &buff) {
    std::string status;
    if (CODE_STATUS.count(code_)) {
        status = CODE_STATUS.find(code_)->second;
    } else {
        code_ = 400;
        status = CODE_STATUS.find(400)->second;
    }
    buff.Append("HTTP/1.1 "+std::to_string(code_) + " " + status + "\r\n");
}

void HttpResponse::AddHeader_ (Buffer &buff) {
    buff.Append("Connection: ");
    if (isKeepAlive_) {
        buff.Append("keep-alive\r\n");
        buff.Append("keep-alive: max=6, timeout=120\r\n");
    } else {
        buff.Append("close\r\n");
    }
    buff.Append("Content-type: " + getFileType_() + "\r\n");
}

void HttpResponse::AddContent_ (Buffer &buff) {
    int srcFd = open((srcDir_ + path_).data(), O_RDONLY);
    // TODO("理解mmap函数的各个参数意义和怎么指定")
    int *mmRet = (int*)mmap(0, mmFileStat_.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    if (*mmRet == -1) {
        ErrorContent(buff, "File NotFound!");
        std::cout<<"File not Found"<<std::endl;
        return;
    }
    mmFile_ = (char*)mmRet;
    close(srcFd);
    buff.Append("Content-length: "+ std::to_string(mmFileStat_.st_size) + "\r\n\r\n");
}

void HttpResponse::UnmapFile () {
    if(mmFile_) {
        munmap(mmFile_, mmFileStat_.st_size);
        mmFile_ = nullptr;
    }
}

std::string HttpResponse::getFileType_ () {
    std::string::size_type idx = path_.find_last_of('.');
    if (idx == std::string::npos) {
        return "text/plain";
    }
    std::string suffix = path_.substr(idx);
    if (SUFFIX_TYPE.count(suffix)) {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}

void HttpResponse::ErrorContent(Buffer & buff, const std::string &message) {
    std::string body;
    std::string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";

    if (CODE_STATUS.count(code_)) {
        status = CODE_STATUS.find(code_)->second;
    } else {
        status = "Bad Request";
    }
    body  += std::to_string(code_) + " : " + status + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";
    

    buff.Append("Contentent-length: " + std::to_string(body.size()) + "\r\n\r\n"); // 有一个空行
    buff.Append(body);
}