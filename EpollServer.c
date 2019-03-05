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
#define EPOLL_QUEUE_LEN 10
#define BUFFER_LENGTH 256

typedef struct ThreadLock
{
    int sd;
    sem_t sem;

} ThreadLock;

ThreadLock* masterList;

sem_t allset_lock;
int epoll_fd;
static struct epoll_event event;

void* serverThread(void* arg);

int main(int argc, char** argv)
{
    int listen_sd, sd, nready;
    socklen_t clientLen;
    struct sockaddr_in server, clientDetails;
    static struct epoll_event events[EPOLL_QUEUE_LEN];
    unsigned count = 0;
    sem_init(&allset_lock, 0, 1);
    masterList = malloc(sizeof(ThreadLock) * TASK_COUNT);

    
    for(int i = 0; i < TASK_COUNT; ++i)
    {
        pthread_t thread;
        masterList[i].sd = -1;
        sem_init(&masterList[i].sem, 0, 0);
        pthread_create(&thread, NULL, serverThread, &masterList[i]);
    }


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


    listen(listen_sd, EPOLL_QUEUE_LEN);

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

    event.events = EPOLLIN | EPOLLERR | EPOLLRDHUP | EPOLLHUP | EPOLLET | EPOLLONESHOT;

    while(1)
    {
        sem_wait(&allset_lock);
        
        nready = epoll_wait(epoll_fd, events, EPOLL_QUEUE_LEN, 0);
        sem_post(&allset_lock);
        for(int i = 0; i < nready; ++i)
        {
            if(events[i].events & (EPOLLHUP | EPOLLERR | EPOLLRDHUP))
            {
                getpeername(events[i].data.fd, (struct sockaddr*)&clientDetails, &clientLen);
                close(events[i].data.fd);
                printf(" Remote Address:  %s:%d closed connection\n", inet_ntoa(clientDetails.sin_addr), ntohs(clientDetails.sin_port));
                printf("Total Connected: %u\n", count);
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
                    printf("Total Connected: %u\n", count);
                }
            }
            else
            {
                int sd = events[i].data.fd;
                int assigned = 0;
                  
                for(int j = 0; j < TASK_COUNT; ++j)
                {
                    if(masterList[j].sd != -1)
                    {
                        continue;
                    }
                    masterList[j].sd = sd;
                    sem_post(&masterList[j].sem);
                    assigned = 1;

                    break;

                }
                     
                if(!assigned)
                {
                    sem_wait(&allset_lock);
                    event.data.fd = sd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, sd, &event );
                    sem_post(&allset_lock);

                }

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
        sem_wait(&lock->sem);
        int sd = lock->sd;
        unsigned messageLength = 0;
        bytesToRead = sizeof(messageLength);
        bp = (char *)&messageLength;
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
        bytesToRead = ntohl(messageLength);
        messageLength = bytesToRead;
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
        send(sd, buffer, messageLength, 0);
        sem_wait(&allset_lock);
        event.data.fd = sd;
        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, sd, &event );
        sem_post(&allset_lock);
        lock->sd = -1;
    }
}
