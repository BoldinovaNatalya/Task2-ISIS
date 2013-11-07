#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>


#define port 3425
#define countOfStr 2
#define countOfCh 50
#define maxUserName 10
#define maxPassword 16
#define maxCommand 1024

int byte_reads;


int main(void)
{
    char* userName = malloc(maxUserName);
    char* password = malloc(maxPassword);
    char* command = malloc(maxCommand);
    char auth[50];

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
    printf("UserName:");
    scanf("%s", userName);
    printf("Password:");
    scanf("%s", password);

    strcpy(auth, "auth ");
    strcat(auth, userName);
    strcat(auth, " ");
    strcat(auth, password);

    char buf[1024];
    send(sock, auth, strlen(auth), 0);
    byte_reads = recv(sock, buf, sizeof(buf), 0);
    //printf("%s", buf);
    if(strcmp(buf, "cor")) {
        while(1) {

            printf("Command:");
            fgets(buf, sizeof(buf), stdin);
            send(sock, buf, strlen(buf), 0);

            while(1) {
                memset(buf, 0, sizeof(buf));
                byte_reads = recv(sock, buf, sizeof(buf), 0);
                if(byte_reads==-1) {
                    break;
                } else {
                    printf(buf);
                    if (strstr(buf, "/")!=NULL){
                        break;
                    }
                }

            }
        }
    }
    close(sock);



    return 0;
}

