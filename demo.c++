#include<sys/socket.h>
#include<netinet/in.h>
#include<memory.h>
                             
int main(){
    short port = 9999;
    int server = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    int flag = 1;
    setsockopt(server,SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    int ret = bind(server, (struct sockaddr *)&address, sizeof(address));
    ret = listen(server, 5);

    return 0;
}