#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>

#include <cstring>
#include <sstream>
#include <unordered_map>

#define MAX_EVENTS 1024
#define LISTENING_PORT 12345
#define RECV_BUFFER_SIZE 1024

int set_nonblock(int fd){
    int flags;
#if defined(O_NONBLOCK)
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    flags = 1;
    return ioctl(fd, FIOBIO, &flags);
#endif
}

void sendMessage(unsigned sender_descriptor, std::unordered_map<unsigned, struct sockaddr_in>& clients, const char* buffer) {
    char ip_address[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clients[sender_descriptor].sin_addr, ip_address, INET_ADDRSTRLEN);
    std::uint16_t port = clients[sender_descriptor].sin_port;

    std::stringstream result_msg;
    result_msg << ip_address << ":" << port << ": " << buffer;

    for(auto& client : clients) {
        if(client.first != sender_descriptor) {
            send(client.first, result_msg.str().c_str(), result_msg.str().length(), MSG_NOSIGNAL);
        }
    }
}

int main(int argc, char *argv[]) {
    int masterSocket = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in sockAddr;
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = htons(12345);
    sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(masterSocket, (struct sockaddr *)(&sockAddr), sizeof(sockaddr));
    set_nonblock(masterSocket);
    listen(masterSocket, SOMAXCONN);

    int ePoll = epoll_create1(0);
    struct epoll_event event;
    event.data.fd = masterSocket;
    event.events = EPOLLIN;
    epoll_ctl(ePoll, EPOLL_CTL_ADD, masterSocket, &event);

    std::unordered_map <unsigned, struct sockaddr_in> clientsMap;
    while (true) {
        struct epoll_event Events[MAX_EVENTS];
        int n = epoll_wait(ePoll, Events, MAX_EVENTS, -1);

        for (unsigned int i = 0; i < n; ++i){
            if (Events[i].data.fd == masterSocket) {
                struct sockaddr_in clientAddr;
                socklen_t client_addr_size = sizeof(clientAddr);

                int slaveSocket = accept(masterSocket, (struct sockaddr *) &clientAddr, &client_addr_size);
                set_nonblock(slaveSocket);
                struct epoll_event tmpEvent;
                tmpEvent.data.fd = slaveSocket;
                tmpEvent.events = EPOLLIN;
                epoll_ctl(ePoll, EPOLL_CTL_ADD, slaveSocket, &tmpEvent);

                clientsMap[slaveSocket] =  clientAddr;
                const char msg[] = "[New client connected]\n";
                sendMessage(slaveSocket, clientsMap, msg);
            }
            else {
                static char buffer[RECV_BUFFER_SIZE];
                memset(buffer, 0, RECV_BUFFER_SIZE);
                int recvResult = recv(Events[i].data.fd, buffer, 1024, MSG_NOSIGNAL);

                if (recvResult == 0 && errno != EAGAIN) {
                    shutdown(Events[i].data.fd, SHUT_RDWR);
                    close(Events[i].data.fd);
                    const char msg[] = "[Client disconnected]\n";
                    sendMessage(recvResult, clientsMap, msg);
                    clientsMap.erase(Events[i].data.fd);
                }
                else if (recvResult > 0) {
                    sendMessage(recvResult, clientsMap, buffer);
                }
            }
        }
    }

    return 0;
}
