#include "sqlconnpool.h"
#include<iostream>

SqlConnPool::SqlConnPool() {
    useCount_ = freeCount_ = 0;
}

SqlConnPool* SqlConnPool::Instance() { // 利用static的特性来实现单例模式
    static SqlConnPool connPool;
    return &connPool;
}

void SqlConnPool::Init(const char *host, int port,
                    const char *user, const char *pwd,
                    const char *dbName, int connSize = 10) {
    assert(connSize > 0);
    for (int i = 0; i < connSize; ++i) {
        MYSQL *sql = nullptr;
        sql = mysql_init(sql);
        if (!sql) {
            std::cerr<<"Mysql Init error"<<std::endl;
            assert(sql); // 自动出错退出
        }
        sql = mysql_real_connect(sql, host,
                            user, pwd, 
                            dbName, port, nullptr, 0);
        if (!sql) {
            std::cerr<<"Mysql Connect error"<<std::endl;
            assert(sql); // 自动出错退出
        }
        connQue_.push(sql);
        
    }
    MAX_CONN_ = connSize;
    sem_init(&semId_, 0, MAX_CONN_); //信号量？
}

MYSQL* SqlConnPool::GetConn() {
    MYSQL *sql = nullptr;
    if (GetFreeConnCount() == 0) {
        std::cerr<<"No free conn, sqlpool in too busy"<<std::endl;
        return nullptr;
    }
    sem_wait(&semId_);
    {
        std::lock_guard<std::mutex> locker(mtx_);
        sql = connQue_.front();
        connQue_.pop();
    }
    return sql;
}

void SqlConnPool::FreeConn(MYSQL *sql) {
    assert(sql);
    std::lock_guard<std::mutex> locker(mtx_);
    connQue_.push(sql);
    sem_post(&semId_);
}

void SqlConnPool::ClosePool() {
    std::lock_guard<std::mutex> locker(mtx_);
    while(!connQue_.empty()) {
        auto item = connQue_.front();
        connQue_.pop();
        mysql_close(item);
    }
    mysql_library_end(); // 关闭数据库？
}

int SqlConnPool::GetFreeConnCount() {
    std::lock_guard<std::mutex> locker(mtx_);
    return connQue_.size();
}

SqlConnPool::~SqlConnPool (){
    ClosePool();
}