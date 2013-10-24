#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

#define port 3425
#define countOfStr 2
#define countOfCh 50

char message[countOfStr][countOfCh] = {"Would you like some tea?\n", "Would you like some coffee?\n"};
char buf[1024];


int main(void)
{
    int sock;
    struct sockaddr_in address;


    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    address.sin_family = AF_INET;
        address.sin_port = htons(port);
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if(connect(sock, (struct sockaddr *)&address, sizeof(address)) < 0)
        {
            perror("connect");
            exit(2);
        }
        srand(time(NULL));
        int r = rand();
        int i = r%2;
        send(sock, message[i], sizeof(message), 0);
        printf(buf);
        recv(sock, buf, sizeof(message), 0);

        printf(message[i]);

        printf(buf);
        close(sock);

    return 0;
}

