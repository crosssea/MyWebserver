#include "webserver.h"

using std::string;
using std::unordered_map;

WebServer::WebServer(int port, int trigMode, int timeoutMS, 
        bool openLinger, int sqlPort, const char *sqlUser,
        const char *sqlPwd, const char *dbName, int connPoolNum,
        int threadNum, bool openLog, int logLevel, int logQueSize) :
        port_(port), openLinger_(openLinger),timeoutMS_(timeoutMS),isClose_(false),
        timer_(new HeapTimer()), threadpool_(new ThreadPool(threadNum)), epoller_(new Epoller())

{
    srcDir_ = getcwd(nullptr, 256);
    std::cout<<srcDir_<<std::endl;
    assert(srcDir_);
    strncat(srcDir_, "/../resources", 15);
    HttpConn::userCount = 0;
    HttpConn::srcDir = srcDir_;
    SqlConnPool::Instance()->Init("localhost", sqlPort, sqlUser, sqlPwd, dbName, connPoolNum);

    InitEventMode_(trigMode);
    if (!InitSocket_()) { isClose_ = true; }

    if (openLog) {
        Log::Instance()->init(logLevel, "./runlog" , ".log", logQueSize);
        if (isClose_) { LOG_ERROR("===Ops Server Init Error"); }
        else {
            LOG_INFO("=======Server Initing ==========");
            LOG_INFO("using Port: %d, OpenLinger: %s", port, openLinger_ ? "True" : "False");
            LOG_INFO("Listen mode: %s, OpenConn mode: %s",
            (listenEvent_ & EPOLLET ? "ET" : "LT"),
            (connEvent_ & EPOLLET ? "ET" : "LT"));
            LOG_INFO("LogSys level: %d", logLevel);
            LOG_INFO("srcDir: %s", HttpConn::srcDir);
            LOG_INFO("SqlCOnnPool num: %d, ThreadPool num: %d", connPoolNum, threadNum);
        }
    }
}

WebServer::~WebServer() {
    close(listenfd_);
    isClose_ = true;
    free(srcDir_);
    SqlConnPool::Instance()->ClosePool();
}

void WebServer::InitEventMode_ (int trigMode) {
    listenEvent_ = EPOLLRDHUP;
    connEvent_ = EPOLLONESHOT | EPOLLRDHUP;

    switch (trigMode) {
        case 0:
            break;
        case 1:
            connEvent_ |= EPOLLET;
            break;
        case 2:
            listenEvent_ |= EPOLLET;
            break;
        case 3:
            listenEvent_ |= EPOLLET;
            connEvent_ |= EPOLLET;
            break;
        default:
            listenEvent_ |= EPOLLET;
            connEvent_ |= EPOLLET;
            break;
    }
    HttpConn::isET  = (connEvent_ & EPOLLET);
}

void WebServer::Start() {
    int timeMS = -1;  /* epoll wait timeout == -1 无事件将阻塞 */
    if(!isClose_) { LOG_INFO("========== Server start =========="); }
    while(!isClose_) {
        if(timeoutMS_ > 0) {
            timeMS = timer_->GetNextTick();
        }
        int eventCnt = epoller_->Wait(timeMS);
        for(int i = 0; i < eventCnt; i++) {
            /* 处理事件 */
            int fd = epoller_->GetEventFd(i);
            uint32_t events = epoller_->GetEvents(i);
            if(fd == listenfd_) {
                DealListen_();
            }
            else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(users_.count(fd) > 0);
                CloseConn_(&users_[fd]);
            }
            else if(events & EPOLLIN) {
                assert(users_.count(fd) > 0);
                // std::cout<<"Deal Read "<<fd<<std::endl;
                DealRead_(&users_[fd]);
            }
            else if(events & EPOLLOUT) {
                assert(users_.count(fd) > 0);
                // LOG_INFO("Beging to produce response for : %d", fd);
                DealWrite_(&users_[fd]);
            } else {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}

void WebServer::SendError_ (int fd, const char *info) {
    assert (fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if (ret < 0) {
        LOG_WARN("send error to client[%d] error!", fd);
    }
    close(fd);
}

void WebServer::CloseConn_ (HttpConn *client){
    assert(client != nullptr);
    LOG_INFO("Client[%d] quit", client->GetFd());
    epoller_->DelFd(client->GetFd());
    client->Close();
}

void WebServer::AddClient_(int fd, sockaddr_in addr) {
    assert(fd > 0);
    users_[fd].Init(fd, addr);
    if (timeoutMS_ > 0) {
        timer_->add(fd, timeoutMS_, std::bind(&WebServer::CloseConn_, this, &users_[fd]));
    }
    epoller_->AddFd(fd, EPOLLIN|connEvent_);
    SetFdNonblock(fd);
    LOG_INFO("Client[%d] in !", fd);
}

void WebServer::DealListen_() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(listenfd_, (struct sockaddr *)&addr, &len);
        if (fd <= 0) {return; } 
        else if (HttpConn::userCount >= MAX_FD){
            SendError_(fd, "Server busy!");
            LOG_WARN("Clients is full!");
            return;
        }
        AddClient_(fd, addr);
    }while(listenEvent_ & EPOLLET);
}

void WebServer::DealRead_(HttpConn *client) {
    assert(client != nullptr);
    ExtentTime_(client);
    // std::cout<<"reading"<<std::endl;
    threadpool_->AddTask(std::bind(&WebServer::OnRead_, this, client));
}

void WebServer::DealWrite_(HttpConn *client) {
    assert(client != nullptr);
    ExtentTime_(client);
    threadpool_->AddTask(std::bind(&WebServer::OnWrite_, this, client));
}

void WebServer::ExtentTime_ (HttpConn *client) {
    assert(client != nullptr);
    if (timeoutMS_ > 0) timer_->adjust(client->GetFd(), timeoutMS_);
}

void WebServer::OnProcess(HttpConn* client) {
    if(client->process()) {
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
    } else {
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLIN);
    }
}

void WebServer::OnRead_ (HttpConn *client) {
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->read(&readErrno);
    // std::cout<<"Readed "<<ret<<std::endl;
    if (ret <= 0 && readErrno != EAGAIN) {
        CloseConn_(client);
        return;
    }
    OnProcess(client);
}

void WebServer::OnWrite_ (HttpConn *client) {
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    // LOG_INFO("User [%d] keep Alive[%d]", client->GetFd(), client->IsKeepAlive());
    ret = client->write(&writeErrno);
    if (client->ToWriteBytes() == 0) {
        if (client->IsKeepAlive()) {
            OnProcess(client);
            return;
        }
    } else if (ret < 0) {
        if (writeErrno == EAGAIN) {
            epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
            return;
        }
    }
    CloseConn_(client);
}

bool WebServer::InitSocket_ () {
    int ret;
    struct sockaddr_in addr;
    if (port_ > 65535 || port_ < 1024) {
        LOG_ERROR("Port: %d is Invalid", port_);
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);

    struct linger optLinger = {0};
    if (openLinger_) { // TODO("linger 优雅关闭，需要背")
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
    }
    listenfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd_ < 0) {
        LOG_ERROR("Init Socket Fail, port is : %", port_);
        return false;
    }
 
    ret = setsockopt(listenfd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if (ret < 0) {
        close(listenfd_);
        LOG_ERROR("Init Linger Error!");
    }
    int optval = 1;
    /*
        复用端口, 最后一个套接字正常收发
    */
    //TODO("看一下端口复用有关的东西,和setsockopt")
    ret = setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));
    if (ret == -1) {
        LOG_ERROR("set socket setsockopt error !");
        close(listenfd_);
        return false;
    }
    ret = bind(listenfd_, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        LOG_ERROR("Bind Port:%d error!", port_);
        close(listenfd_);
        return false;
    }
    ret = listen(listenfd_, 6);
    if(ret < 0) {
        LOG_ERROR("Listen port:%d error!", port_);
        close(listenfd_);
        return false;
    }
    ret = epoller_->AddFd(listenfd_, listenEvent_ | EPOLLIN);
    if (ret == 0) {
        LOG_ERROR("Add listen error!");
        close(listenfd_);
        return false;
    }
    SetFdNonblock(listenfd_);
    // LOG_INFO("Server port:%d", port_);
    return true;
}

int WebServer::SetFdNonblock (int fd) {
    assert(fd>0);
    // TODO("设置O_NONBLOCK以及fcntl参数")
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}