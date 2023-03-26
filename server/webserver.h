#ifndef WEBSERVER_H
#define WEBSERVER_H

#include<unordered_map>
#include<sys/socket.h>
#include<fcntl.h>
#include<unistd.h>
#include<assert.h>
#include<errno.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<arpa/inet.h>

#include "../pool/ThreadPool.h"
#include "../pool/sqlconnRAII.h"
#include "epoller.h"
#include "../timer/heaptimer.h"
#include "../http/httpconn.h"
#include "../pool/sqlconnpool.h"

class WebServer{
public:
    WebServer(
        int port, int trigMode, int timeoutMS, 
        bool openLinger, int sqlPort, const char *sqlUser,
        const char *sqlPwd, const char *dbName, int connPoolNum,
        int threadNum, bool openLog, int logLevel, int logQueSize);
    ~WebServer();
    void Start();
private:
    bool InitSocket_();
    void InitEventMode_(int trigMode);
    void AddClient_(int fd, sockaddr_in addr);

    void DealListen_();
    void DealWrite_(HttpConn *);
    void DealRead_(HttpConn *);

    void SendError_(int fd, const char *info);
    void ExtentTime_(HttpConn *);
    void CloseConn_(HttpConn *);

    void OnRead_(HttpConn *);
    void OnWrite_(HttpConn *);
    void OnProcess(HttpConn *);

    static const int MAX_FD = 65536;

    static int SetFdNonblock(int fd);

    int port_;
    bool openLinger_;
    int timeoutMS_;
    bool isClose_;
    int listenfd_;
    char *srcDir_;


    uint32_t listenEvent_;
    uint32_t connEvent_;



    std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConn> users_;
};



#endif