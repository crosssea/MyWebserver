/*
    @Description: timer control using min heap
*/
#ifndef HEAPTIMER_H
#define HEAPTIMER_H

#include<queue>
#include<unordered_map>
#include<time.h>
#include<algorithm>
#include<arpa/inet.h>
#include<functional>
#include<assert.h>
#include<chrono>

#include "../log/log.h"

using TimeOutCallBack = std::function<void()>;
using Clock = std::chrono::high_resolution_clock;
using MS = std::chrono::milliseconds;
using TimeStamp = Clock::time_point;

struct TimerNode{
    TimerNode() = default;
    TimerNode(int id_, const TimeStamp &expires_, const TimeOutCallBack &cb_):id(id_), expires(expires_), cb(cb_){ };
    int id;
    TimeStamp expires;
    TimeOutCallBack cb;
    bool operator < (const TimerNode &rhs) {
        return expires < rhs.expires;
    }
};

class HeapTimer { //单线程的吗
private:
    void del_(size_t i);
    void siftup_(size_t i);
    bool siftdown_(size_t index, size_t n);
    void SwapNode_(size_t i, size_t j);

    std::vector<TimerNode> heap_;

    std::unordered_map<int, size_t> ref_; //不知道有什么用，等后面在看
public:
    HeapTimer() { heap_.reserve(64); } //  reserve不掉用构造函数，而resize会掉用

    ~HeapTimer() { clear(); }

    void adjust(int id, int newExpires);
    void add(int id, int timeOut, const TimeOutCallBack &cb);

    void doWork(int id); //因为不一定是超时才work， 所以需要下面的umap去移除这个事件

    void clear();

    void tick();

    void pop();

    int GetNextTick();

};

#endif