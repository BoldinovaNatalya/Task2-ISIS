#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

#define port 3425
#define sizeOfBuf 1024

int main(void) {

    int sock, listener;
    struct sockaddr_in addr;
    char buf[sizeOfBuf];
    char* answer;
    int bytes_read;

    listener = socket(AF_INET, SOCK_STREAM, 0);
    if(listener < 0)
    {
        perror("socket");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        exit(2);
    }

    listen(listener, 1);

    while(1)
    {
        sock = accept(listener, NULL, NULL);
        if(sock < 0)
        {
            perror("accept");
            exit(3);
        }

        while(1)
        {
            bytes_read = recv(sock, buf, 1024, 0);
            if(bytes_read <= 0) {

                break;

            } else {
                if(strstr(buf, "tea")!=NULL) {
                    answer = "Yes\n";
                } else {
                    answer = "No\n";
                }

            }

            send(sock, answer, bytes_read, 0);
        }

        close(sock);
    }
    return 0;
}

