/*
 * Simple Echo-server
*/
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <unistd.h>

using namespace std;

int main() {

    static const int MY_PORT = 12345;
    static const int BUFFER_SIZE = 2048;

    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(MY_PORT);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    int listen_sock  = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_sock == -1) {
        printf("Could not create listen socket\n");
        return 1;
    }

    if ((bind(listen_sock, (struct sockaddr *)&sa, sizeof(sa))) == -1) {
        printf("Could not bind socket\n");
        close(listen_sock);
        return 1;
    }

    if (listen(listen_sock, SOMAXCONN) == -1) {
        printf("Error on start listen server socket\n");
        close(listen_sock);
        return 1;
    }

    int conn;
    int nRecv=1;
    char buffer[BUFFER_SIZE];

    printf("Ready for connection...\n");

    while (true) {
        conn = accept(listen_sock, 0, 0);
        while(nRecv > 0) {
            nRecv = recv(conn, buffer, BUFFER_SIZE, 0);
            if(nRecv > 0) {
                send(conn, buffer, nRecv, 0);
                printf("    >%s\n", buffer);
            }
        }
        shutdown(conn, SHUT_RDWR);
        close(conn);
    }
    close(listen_sock);
    printf("Client disconnected\n");

    return 0;
}
