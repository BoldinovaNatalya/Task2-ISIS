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
#include <sys/stat.h>
#include <unistd.h>

#include <sys/sem.h>
#include <sys/ipc.h>




#define port 3425
#define sizeOfBuf 1024
#define MAX_NUMBER_OF_EVENTS 64
#define countOfChildren 4

struct TaskStruct {
    int socket;
    char data[sizeOfBuf];
};

typedef struct TaskStruct* Task;

union semun {
    int val; /* used for SETVAL only */
    struct semid_ds *buf; /* for IPC_STAT and IPC_SET */
    ushort *array; /* used for GETALL and SETALL */
};

key_t key;
int semid;
union semun arg;
struct sembuf sbLockRead = {0, -1, 0};
struct sembuf sbUnlockRead = {0, 1, 0};
struct sembuf sbLockWrite = {1, -1, 0};
struct sembuf sbUnlockWrite = {1, 1, 0};

void process_socket(int wd, Task task) {
    FILE *f;
    char auth[100];
    char* authResult;
    //char answer[sizeOfBuf];
    Task answer = (Task)malloc(sizeof(struct TaskStruct));;
    char userPassword[sizeOfBuf];
    answer->socket = task->socket;
    if(strncmp(task->data, "auth", 4)==0) {
        f = fopen("UserPassword", "r");
        while(fgets(userPassword, sizeof(userPassword), f)) {
            strcpy(auth, "auth ");
            strcat(auth, userPassword);
            if(!strncmp(auth, task->data, strlen(task->data))) {
                authResult = "cor";
            } else {
                authResult = "incor";
            }
        }
        printf("beforelock %d\n",  semctl(semid, 0, GETVAL, arg));
        strcpy(answer->data, authResult);
        //sb.sem_op = -1;
        if (semop(semid, &sbLockWrite, 1) == -1) {
            perror("semop");
            exit(1);
        }
        printf("lock\n");
        write(wd, answer, sizeof(struct TaskStruct));

        //sb.sem_op = 1;
        if (semop(semid, &sbUnlockWrite, 1) == -1) {
            perror("semop");
            exit(1);
        }


        printf("%d %s\n", answer->socket, answer->data);
    } else {
        f = popen(task->data, "r");

        // sb.sem_op = -1;
        if (semop(semid, &sbLockWrite, 1) == -1) {
            perror("semop");
            exit(1);
        }
        while(fgets(answer->data, sizeof(answer->data),f)) {
            printf("%d %s\n", answer->socket, answer->data);
            write(wd, answer, sizeof(struct TaskStruct));
        }

        strcpy(answer->data, "/\0");

        printf("%d %s\n", answer->socket, answer->data);
        write(wd, answer, sizeof(struct TaskStruct));

        if (semop(semid, &sbUnlockWrite, 1) == -1) {
            perror("semop");
            exit(1);
        }



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

    if(key = ftok("some", 'K')== -1) {
        perror("ftok");
        exit(1);
    }

    if(semid = semget(key, 2, 0666 | IPC_CREAT)==-1) {
        perror("semget");
        exit(2);
    }

    arg.val = 1;

    if(semctl(semid, 0, SETVAL, arg) <0) {
        perror("semctl");
        exit(2);
    }


    arg.val = 1;
    if(semctl(semid, 1, SETVAL, arg) <0) {
        perror("semctl");
        exit(2);
    }
    printf("after SETVAL %d\n", semctl(semid, 0, GETVAL, arg));


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

    Task task = (Task)malloc(sizeof(struct TaskStruct));

    mkfifo("fifo_in", S_IFIFO| 0666);
    mkfifo("fifo_out", S_IFIFO| 0666);


    int k;
    int od, wd;
    pid_t pids[countOfChildren];
    int pid;
    for(k = 0; k<=countOfChildren; k++) {
        pid = fork();
        if(pid ==0) {
            char buf[sizeOfBuf];
            char data[sizeOfBuf];
            od = open("fifo_in", O_RDONLY);

            wd = open("fifo_out", O_WRONLY);
            printf("before readlock %d \n",  semctl(semid, 0, GETVAL, arg));
            while(1) {
                //sb.sem_op = -1;
                if (semop(semid, &sbLockRead, 1) == -1) {
                    perror("semop");
                    exit(1);
                }

                printf(" after readlock %d \n",  semctl(semid, 0, GETVAL, arg));

                read(od, task, sizeof(struct TaskStruct));

                //sb.sem_op = 1;

                // printf("%d\n", sb.sem_op);
                if (semop(semid, &sbUnlockRead, 1) == -1) {
                    perror("semop");
                    exit(1);
                }
                //sb.sem_op = -1;

                printf(" before process socket lock %d\n",  semctl(semid, 0, GETVAL, arg));

                printf("42 %d %s\n",task->socket, task->data);

                //write(wd, task, sizeof(struct TaskStruct));
                process_socket(wd, task);
            }
        }
        pids[k] = pid;
    }






    od = open("fifo_in", O_WRONLY);
    wd = open("fifo_out", O_RDONLY);
    make_socket_non_blocking(wd);
    event.data.fd = wd;
    event.events = EPOLLIN | EPOLLET;


    s = epoll_ctl(efd, EPOLL_CTL_ADD, wd, &event);
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

            } else if (wd != events[i].data.fd){
                task->socket = events[i].data.fd;
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
                    //task->data = buf;
                    strcpy(task->data, buf);
                    int sockID = 1;
                    printf("%d %s\n", task->socket, task->data);
                    int r = write(od, task, sizeof(struct TaskStruct));
                    printf("%d\n", r);
                    //write(od, task->data, sizeof(task->data));
                    //read(wd, task, sizeof(struct TaskStruct));

                    // send(task->socket, task->data, strlen(task->data), 0);

                    printf("%d %s\n", task->socket, task->data);

                    // process_socket(events[i].data.fd, buf);
                }
            } else {
                while(1)
                {

                    bytes_read = read(events[i].data.fd, task, sizeof(struct TaskStruct));
                    if(bytes_read <= 0) {

                        if(errno != EAGAIN) {
                            close(events[i].data.fd);
                        }
                        break;
                    } else {

                        send(task->socket, task->data, strlen(task->data), 0);
                    }

                }

            }
        }

    }

    return 0;
}

