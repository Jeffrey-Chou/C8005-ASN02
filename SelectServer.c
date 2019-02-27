#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <semaphore.h>



#define PORT 7000
#define TASK_COUNT 5

typedef struct threadList
{
    int sock;
    sem_t sem;

} threadList;

int main(int argc, char** argv)
{
    int listen_sd, sd, clients[FD_SETSIZE],max_fd, nready;
    socklen_t clientLen;
    struct sockaddr_in server, clientDetails;
    fd_set rset, allset;

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


    listen(listen_sd, 20);
    max_fd = listen_sd;
    for(int i = 0; i < FD_SETSIZE; ++i)
    {
        clients[i] = -1;
    }
    FD_ZERO(&allset);
    FD_SET(listen_sd, &allset);
    while(1)
    {
        rset = allset;
        nready = select(max_fd + 1, &rset, NULL, NULL, NULL);

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
                    FD_SET(sd, &allset);
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

        
    }
    return 0;
}