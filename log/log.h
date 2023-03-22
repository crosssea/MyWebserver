#ifndef LOG_H
#define LOG_H

#include<mutex>
#include<string>
#include<thread>
#include<sys/time.h>
#include<string.h>
// vastart va_end  可变长参数有关?
#include<stdarg.h>
#include<assert.h>
#include<sys/stat.h> // mkdir 系统调用？
#include "blockqueue.h"
#include "../buffer/buffer.h"


class Log {
public:
    void init(int level, const char *path = "./log",
                const char *suffix = ".ll_log",
                int maxQueueCapacity = 1024);
    static Log* Instance(); //单例模式
    static void FlushLogThread();

    void write(int level, const char *format, ...);
    void flush();

    int GetLevel();
    void SetLevel(int level);
    bool IsOpen() {return isOpen_;}

private:
    Log();
    virtual ~Log(); //虚函数， 存在子类？
    void AsyncWrite();
private:
    static const int LOG_PATH_LEN = 256;
    static const int LOG_NAME_LEN = 256;
    static const int MAX_LINES = 50000;

    const char *path_;
    const char *suffix_;

    int MAX_LINES_;
    int lineCount_;
    int toDay_;
    bool isOpen_;

    Buffer buff_;
    int level_;
    bool isAsync_;

    FILE *fp_;
    std::unique_ptr<BlockQueue<std::string>> deque_;
    std::unique_ptr<std::thread> writeThread_; //这个的作用是什么
    std::mutex mtx_;

};

#define LOG_BASE(level, format, ...) \
    do { \
        Log *log = Log::Instance(); \
        if (log->IsOpen() && log->GetLevel() <= level) {\
            log->write(level, format, ##__VA_ARGS__); \
            log->flush;\
        }\
    } while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#endif