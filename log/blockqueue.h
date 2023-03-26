#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H

#include<mutex>
#include<deque>
#include<condition_variable>
#include<sys/time.h>
#include<assert.h>

template<class T>
class BlockQueue{
public:
    explicit BlockQueue(size_t MaxCapacity = 1000);

    ~BlockQueue();

    void clear();

    bool empty();

    bool full();

    void Close();

    size_t size();

    size_t capacity();

    void push_back(const T &item);

    void push_front(const T &item);

    T front();

    T back();

    bool pop(T &item);

    bool pop(T &item, int timeout);

    void flush();

private:
    std::deque<T> deq_;
    size_t capacity_;
    std::mutex mtx_;
    bool isClose_;
    std::condition_variable condConsuer_;
    std::condition_variable condProducer_;
};

template<class T>
BlockQueue<T>::BlockQueue(size_t MaxCapacity):capacity_(MaxCapacity) {
    assert(MaxCapacity > 0);
    isClose_ = false;
}

template<class T>
BlockQueue<T>::~BlockQueue() {
    Close();
}

template<class T>
void BlockQueue<T>::Close() {
    {
        std::lock_guard<std::mutex> locker(mtx_);
        deq_.clear(); // 不会丢失？
        isClose_ = true;
    }
    condConsuer_.notify_all();
    condProducer_.notify_all();
}

template<class T>
void BlockQueue<T>::flush() {
    condConsuer_.notify_one(); // 唤醒一个线程来写这些log？
}

template<class T>
void BlockQueue<T>::clear(){
    std::lock_guard<std::mutex> locker(mtx_);
    deq_.clear();
}

template<class T>
T BlockQueue<T>::front(){
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.front();
}

template<class T>
T BlockQueue<T>::back() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.back();
}

template<class T>
size_t BlockQueue<T>::size() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size();
}

template<class T>
size_t BlockQueue<T>::capacity() {
    std::lock_guard<std::mutex> locker(mtx_);
    return capacity_;
}

template<class T>
bool BlockQueue<T>::full(){
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size() >= capacity_;
}

template<class T>
void BlockQueue<T>::push_back(const T &item) {
    std::unique_lock<std::mutex> locker(mtx_);
    while (deq_.size() >= capacity_)
    {
        condProducer_.wait(locker);
    }
    deq_.push_back(item);
    condConsuer_.notify_one(); // 通知来取？
}

template<class T>
void BlockQueue<T>::push_front(const T &item) {
    std::unique_lock<std::mutex> locker(mtx_);
    while (deq_.size() >= capacity_)
    {
        condProducer_.wait(locker);
    }
    deq_.push_front(item);
    condConsuer_.notify_one(); // 通知来取？
}

template<class T>
bool BlockQueue<T>::empty() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.empty();
}

template<class T>
bool BlockQueue<T>::pop(T &item) {
    std::unique_lock<std::mutex> locker(mtx_);
    while (deq_.empty()) {
        condConsuer_.wait(locker);
        if (isClose_) return false;
    }
    item = deq_.front();
    deq_.pop_front();
    condProducer_.notify_one(); //通知一个可以放入的位置了
    return true;
}

template<class T>
bool BlockQueue<T>::pop(T &item, int timeOut) {
    std::unique_lock<std::mutex> locker(mtx_);
    while (deq_.empty()) {
        if (condConsuer_.wait_for(locker, std::chrono::seconds(timeOut))
                == std::cv_status::timeout) {
                    return false;
                };
        if (isClose_) return false;
    }
    item = deq_.front();
    deq_.pop_front();
    condProducer_.notify_one(); //通知一个可以放入的位置了
    return true;
}
#endif