#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <assert.h>
#include <errno.h>

#define PORT 8000
#define TASK_COUNT 5
#define EPOLL_QUEUE_LEN 256
#define BUFFER_LENGTH 256

typedef struct ThreadLock
{
    int sd;
    sem_t sem;

} ThreadLock;

ThreadLock* masterList;

sem_t allset_lock;

void* serverThread(void* arg);

int main(int argc, char** argv)
{
    int listen_sd, sd, clients[FD_SETSIZE],max_fd, nready, epoll_fd, bytesToRead;
    socklen_t clientLen;
    struct sockaddr_in server, clientDetails;
    static struct epoll_event events[EPOLL_QUEUE_LEN], event;
    char *bp;
    char buffer[BUFFER_LENGTH];
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    //sem_init(&allset_lock, 0, 1);
    masterList = malloc(sizeof(ThreadLock) * TASK_COUNT);

    /*
    for(int i = 0; i < TASK_COUNT; ++i)
    {
        pthread_t thread;
        masterList[i].sd = -1;
        sem_init(&masterList[i].sem, 0, 0);
        pthread_create(&thread, NULL, serverThread, &masterList[i]);
    }*/


    if((listen_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        return 1;
    }
    int option = 1;
    if(setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == -1)
    {
        return 1;
    }

    if( fcntl(listen_sd, F_SETFL, O_NONBLOCK | fcntl(listen_sd, F_GETFL, 0)) == -1)
    {
        return 1;
    }
    memset(&clientDetails, 0, sizeof(struct sockaddr_in));
    memset(&server, 0, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(listen_sd, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        return 1;
    }


    listen(listen_sd, 20);
    max_fd = listen_sd;
    for(int i = 0; i < FD_SETSIZE; ++i)
    {
        clients[i] = -1;
    }
    epoll_fd = epoll_create(EPOLL_QUEUE_LEN);
    if(epoll_fd == -1)
    {
        return 1;
    }

    event.events = EPOLLIN | EPOLLERR | EPOLLRDHUP | EPOLLHUP | EPOLLET;
    event.data.fd = listen_sd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sd, &event) == -1)
    {
        return 1;
    }

    while(1)
    {
        //sem_wait(&allset_lock);
        //sem_post(&allset_lock);
        nready = epoll_wait(epoll_fd, events, EPOLL_QUEUE_LEN, -1);

        for(int i = 0; i < nready; ++i)
        {
            if(events[i].events & (EPOLLHUP | EPOLLERR | EPOLLRDHUP))
            {
                close(events[i].data.fd);
                printf(" Remote Address:  %s:%d closed connection\n", inet_ntoa(clientDetails.sin_addr), ntohs(clientDetails.sin_port));
                //clients[i] = -1;
                continue;
            }


            assert (events[i].events & EPOLLIN);

            if(events[i].data.fd == listen_sd)
            {
                while(1)
                {
                clientLen = sizeof(struct sockaddr_in);
                sd = accept(listen_sd, (struct sockaddr *)&clientDetails, &clientLen);

                if(sd == -1)
                {
                    if(errno != EAGAIN && errno != EWOULDBLOCK)
                    {

                    }
                    break;
                }
                if(fcntl(sd, F_SETFL, O_NONBLOCK | fcntl(sd, F_GETFL)) == -1)
                {
                    fprintf(stderr, "died in fcntl\n");
                    return 1;
                }

                event.data.fd = sd;
                if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sd, &event) == -1)
                {
                    fprintf(stderr, "died in epollctl\n");
                    return 1;
                }

                printf(" Remote Address:   %s:%d\n", inet_ntoa(clientDetails.sin_addr), ntohs(clientDetails.sin_port));
                }
            }
            else
            {
                int sd = events[i].data.fd;
                ssize_t n;
                unsigned messageLength = 0;
                bp = (char *)&messageLength;





                    bytesToRead = sizeof(messageLength);
                    while(1)
                    {
                        n = recv(sd, bp, bytesToRead, 0);
                        if(n <= 0)
                        {
                            if(bytesToRead > 0)
                            {
                                continue;
                            }
                            else
                            {
                                break;
                            }
                        }
                        bp += n;
                        bytesToRead -= n;
                    }
                    printf("n is %d\n", n);
                    bytesToRead = ntohl(messageLength);
                    messageLength = bytesToRead;
                    printf("message length is %u\n", bytesToRead);
                    bp = buffer;
                    while(1)
                    {
                        n = recv(sd, bp, bytesToRead, 0);
                        if(n <= 0)
                        {
                            if(bytesToRead > 0)
                            {
                                continue;
                            }
                            else
                            {
                                break;
                            }
                        }
                        bp += n;
                        bytesToRead -= n;
                    }
                    printf("received:\n%s\n", buffer);
                    send(sd, buffer, messageLength, 0);
                    /*
                   for(int i = 0; i < TASK_COUNT; ++i)
                   {
                       if(masterList[i].sd != -1)
                       {
                           continue;
                       }
                       masterList[i].sd = sd;
                       sem_post(&masterList[i].sem);
                       sem_wait(&allset_lock);
                       FD_CLR(sd, &allset);
                       sem_post(&allset_lock);
                       break;

                   }*/

            }


        }
    }
    return 0;
}

void* serverThread(void* arg)
{
    ThreadLock* lock = (ThreadLock*) arg;
    char buffer[BUFFER_LENGTH], *bp;
    int bytesToRead;
    ssize_t n;
    while(1)
    {
        //sem_wait(&lock->sem);
        unsigned messageLength = 0;
        bp = (char *)&messageLength;
        bytesToRead = sizeof(messageLength);
        while( (n = read(lock->sd, bp, bytesToRead)) > 0)
        {
            bp += n;
            bytesToRead -= n;
        }
        bytesToRead = ntohl(messageLength);
        messageLength = bytesToRead;
        printf("message length is %u\n", bytesToRead);
        bp = buffer;
        while((n = read(lock->sd, bp, bytesToRead)) > 0)
        {
            bp += n;
            bytesToRead -= n;
        }
        printf("received:\n%s\n", buffer);
        write(lock->sd, buffer, messageLength);
        //sem_wait(&allset_lock);
        //FD_SET(lock->sd, &allset);
        //sem_post(&allset_lock);
        lock->sd = -1;
    }
}
