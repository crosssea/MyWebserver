#include "log.h"

Log::Log() {
    lineCount_ = 0; //当前已写入的log行数
    isAsync_ = false; // 是否异步写入
    writeThread_ = nullptr; 
    deque_ = nullptr;
    toDay_ = 0;
    fp_ = nullptr;
}

Log::~Log() {
    if (writeThread_ && writeThread_->joinable()) {
        while(!deque_->empty()){
            deque_->flush();
        }
        deque_->Close();
        writeThread_->join();
    }
    if (fp_) {
        std::lock_guard<std::mutex> locker(mtx_);
        flush();
        fclose(fp_);
    }
}

int Log::GetLevel() {
    std::lock_guard<std::mutex> locker(mtx_);
    return level_;
}

void Log::SetLevel(int level) {
    std::lock_guard<std::mutex> locker(mtx_);
    level_ = level;
}

void Log::init(int level = 1, const char *path, 
                const char *suffix, int maxQueueSize) {
    isOpen_ = true;
    level_ = level;
    if (maxQueueSize > 0) {
        isAsync_ = true;
        if (!deque_) {
            deque_ = std::move(std::unique_ptr<BlockQueue<std::string>> (new BlockQueue<std::string>()));
            writeThread_ = std::move(std::unique_ptr<std::thread>(new std::thread(FlushLogThread)));
        }
    } else {
        isAsync_ = false; //有就要写
    }
    lineCount_ = 0;

    time_t time = std::time(nullptr);
    struct tm *sysTime = localtime(&time);
    struct tm t = *sysTime;
    path_ = path;
    suffix_ = suffix;
    char fileName[LOG_NAME_LEN] = {0};
    snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s", 
            path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
    toDay_ = t.tm_mday;

    {
        std::lock_guard<std::mutex> locker(mtx_);
        buff_.RetrieveAll();
        if (fp_) {
            flush();
            fclose(fp_);
        }
        //TODO("理解第二个参数权限是什么意思")
        fp_ = fopen(fileName, "a"); 
        if (fp_ == nullptr) {
            mkdir(path_, 0777);//文件夹权限是第二个参数
            fp_ = fopen(fileName, "a");
        }
        assert(fp_ != nullptr);
    }
}

void Log::write(int level, const char *format, ...) {
    
}