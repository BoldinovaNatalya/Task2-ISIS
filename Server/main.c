#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>




#define port 3425
#define sizeOfBuf 1024
#define MAX_NUMBER_OF_EVENTS 64

void process_socket(int fd, char* buf) {
    FILE *f;
    char auth[100];
    char* authResult;
    char answer[sizeOfBuf];
    char userPassword[sizeOfBuf];
    if(strncmp(buf, "auth", 4)==0) {
        f = fopen("UserPassword", "r");
        while(fgets(userPassword, sizeof(userPassword), f)) {
            strcpy(auth, "auth ");
            strcat(auth, userPassword);
            if(!strncmp(auth, buf, strlen(buf))) {
                authResult = "cor";
            } else {
                authResult = "incor";
            }
        }
        write(fd, authResult, strlen(authResult));
    } else {
        f = popen(buf, "r");
        while(fgets(answer, sizeof(answer),f)) {
            write(fd, answer, strlen(answer));
        }

        answer[0] = '/';
        answer[1] = '\0';
        write(fd, answer, strlen(answer));
        pclose(f);
    }
}


static int make_socket_non_blocking (int sfd)
{
    int flags, s;

    flags = fcntl (sfd, F_GETFL, 0);
    if (flags == -1)
    {
        return -1;
    }

    flags |= O_NONBLOCK;
    s = fcntl (sfd, F_SETFL, flags);
    if (s == -1)
    {
        return -1;
    }

    return 0;
}


int main(void) {

    signal(SIGPIPE, SIG_IGN);
    int s;
    int efd;
    struct epoll_event event;
    struct epoll_event* events;
    events = malloc(sizeof(struct epoll_event)*MAX_NUMBER_OF_EVENTS);

    int sock, listener;
    struct sockaddr_in addr;
    char buf[sizeOfBuf];   
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

    s = make_socket_non_blocking(listener);
    listen(listener, 1);

    if(s==-1) {
        perror("listen");
        exit(3);
    }


    efd = epoll_create1(0);
    if(efd==-1) {
        perror("epoll");
        exit(4);
    }

    event.data.fd = listener;
    event.events = EPOLLIN | EPOLLET;

    s = epoll_ctl(efd, EPOLL_CTL_ADD, listener, &event);
    if(s==-1) {
        perror("epoll_ctl");
        exit(5);
    }

    while(1) {
        int active, i;
        active = epoll_wait(efd, events, MAX_NUMBER_OF_EVENTS, -1);
        for(i = 0; i<active; i++) {
            if(events[i].events & EPOLLERR || events[i].events & EPOLLHUP || !((events[i].events & EPOLLIN))) {
                perror("epoll error /n");
                close(events[i].data.fd);
                continue;
            } else if (listener == events[i].data.fd) {
                while(1) {

                    sock = accept(listener, NULL, NULL);
                    if(sock ==-1)
                    {if ((errno == EAGAIN) ||
                                (errno == EWOULDBLOCK))
                        {
                            /* We have processed all incoming
                                   connections. */
                            break;
                        }
                        else
                        {
                            perror ("accept");
                            break;
                        }
                    }

                    s = make_socket_non_blocking(sock);
                    if(s==-1) {
                        perror("make_socket_non_blocking");
                        exit(7);
                    }
                    event.data.fd = sock;
                    event.events = EPOLLIN | EPOLLET;
                    s = epoll_ctl(efd, EPOLL_CTL_ADD, sock, &event);
                    if (s == -1) {
                        perror("ctl");
                        exit(8);
                    }
                }

            } else {
                int done = 0;


                memset(buf, 0, sizeof(buf));
                while(1)
                {

                    bytes_read = recv(events[i].data.fd, buf, 1024, 0);
                    if(bytes_read <= 0) {

                        if(errno != EAGAIN) {
                            done = 1;
                            close(events[i].data.fd);
                        }

                        break;
                    }
                }
                if(!done) {
                    process_socket(events[i].data.fd, buf);
                }
            }
        }

    }

    return 0;
}

