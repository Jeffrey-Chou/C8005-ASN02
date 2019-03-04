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


#define PORT 8000
#define TASK_COUNT 5
#define BUFFER_LENGTH 256

typedef struct ThreadLock
{
    int sd;
    sem_t sem;

} ThreadLock;

ThreadLock* masterList;
fd_set allset;
sem_t allset_lock;

void* serverThread(void* arg);

int main(int argc, char** argv)
{
    int listen_sd, sd, clients[FD_SETSIZE],max_fd, nready;
    socklen_t clientLen;
    struct sockaddr_in server, clientDetails;
    fd_set rset;
    char *bp;
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
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
    memset(&clientDetails, 0, sizeof(struct sockaddr_in));
    memset(&server, 0, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(listen_sd, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        return 1;
    }


    listen(listen_sd, 100);
    max_fd = listen_sd;
    for(int i = 0; i < FD_SETSIZE; ++i)
    {
        clients[i] = -1;
    }
    FD_ZERO(&allset);
    FD_SET(listen_sd, &allset);
    while(1)
    {
        sem_wait(&allset_lock);
        rset = allset;
        sem_post(&allset_lock);
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
        nready = select(max_fd + 1, &rset, NULL, NULL, &timeout);
        

        if(nready == 0)
        {
            //printf("nothing to select\n");
            continue;
        }

        if (FD_ISSET(listen_sd, &rset))
        {
            clientLen = sizeof(struct sockaddr_in);
            if((sd = accept(listen_sd, (struct sockaddr *)&clientDetails, &clientLen)) == -1)
            {
                return 1;
            }

            printf(" Remote Address:   %s:%d\n", inet_ntoa(clientDetails.sin_addr), ntohs(clientDetails.sin_port));

            for(int i = 0; i < FD_SETSIZE; ++i)
            {
                if(clients[i] < 0)
                {
                    clients[i] = sd;
                    sem_wait(&allset_lock);
                    FD_SET(sd, &allset);
                    sem_post(&allset_lock);
                    if(sd > max_fd)
                    {
                        max_fd = sd;
                    }
                    break;
                }
            }

            if(--nready <= 0)
            {
                continue;
            }
        }

        for(int i = 0; i < FD_SETSIZE; ++i)
        {
            if((sd = clients[i]) < 0)
            {
                continue;
            }

            if(FD_ISSET(sd, &rset))
            {
                ssize_t n;
                unsigned messageLength = 0;
                bp = (char *)&messageLength;
                ioctl(sd, FIONREAD, &n);
                if(n == 0)
                {
                    printf(" Remote Address:  %s:%d closed connection\n", inet_ntoa(clientDetails.sin_addr), ntohs(clientDetails.sin_port));
					close(sd);
                    sem_wait(&allset_lock);
                    FD_CLR(sd, &allset);
                    sem_post(&allset_lock);
                    clients[i] = -1;

                }
                else
                {
                    /*
                    bytesToRead = sizeof(messageLength);
                    while( (n = read(sd, bp, bytesToRead)) > 0)
                    {
                        bp += n;
                        bytesToRead -= n;
                    }
                    bytesToRead = ntohl(messageLength);
                    messageLength = bytesToRead;
                    printf("message length is %u\n", bytesToRead);
                    bp = buffer;
                    while((n = read(sd, bp, bytesToRead)) > 0)
                    {
                        bp += n;
                        bytesToRead -= n;
                    }
                    printf("received:\n%s\n", buffer);
                    write(sd, buffer, messageLength);
                    */
                   int assigned = 0;
                   while(!assigned)
                   {
                   for(int j = 0; j < TASK_COUNT; ++j)
                   {
                       if(masterList[j].sd != -1)
                       {
                           continue;
                       }
                       assigned = 1;
                       masterList[j].sd = sd;
                       sem_wait(&allset_lock);
                       FD_CLR(sd, &allset);
                       sem_post(&allset_lock);
                       sem_post(&masterList[j].sem);
                       
                       break;

                   }
                   //printf("waiting to be assigned\n");
                   }
                   //printf("got assigned\n");
                
                }
                if (--nready <= 0)
                {
                    break;
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
        printf("before lock\n");
        sem_wait(&lock->sem);
        unsigned messageLength = 0;
        bp = (char *)&messageLength;
        bytesToRead = sizeof(messageLength);
        while( (n = recv(lock->sd, bp, bytesToRead, 0)) < bytesToRead)
        {
            bp += n;
            bytesToRead -= n;
        }
        bytesToRead = ntohl(messageLength);
        messageLength = bytesToRead;
        printf("message length is %u\n", bytesToRead);
        bp = buffer;
        while((n = recv(lock->sd, bp, bytesToRead,0)) < messageLength)
        {
            bp += n;
            bytesToRead -= n;
        }
        printf("received:\n%s\n", buffer);
        send(lock->sd, buffer, messageLength, 0);
        sem_wait(&allset_lock);
        FD_SET(lock->sd, &allset);
        sem_post(&allset_lock);
        lock->sd = -1;
    }
}