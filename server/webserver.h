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

#include<../ThreadPool.h>

class WebServer{
public:
    WebServer(
        int port, int trigMode, int timoutMS, bool OptLinger = false
    );
    ~WebServer();
    void Start();
private:
    bool InitSocket();
    void InitEventMode(int trigMode);
    void AddClient(int fd, sockaddr_in addr);


    int port_;
    bool openLinger_;
    int timeoutMS_;
    bool isClose_;
    int listenfd_;

    uint32_t listenEvent_;
    uint32_t connEvent_;

    std::unique_ptr<ThreadPool> threadpool_;
    // std::unique_ptr
    // std::
};



#endif