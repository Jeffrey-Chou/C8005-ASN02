#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define SERVER_PORT 7000
#define OPTIONS "?s:p:i:l:t:"

void* clientThread(void* threadArg);
void* generateMessage(unsigned messageLength);


struct sockaddr_in server;
unsigned iteration;
char* message;

int main(int argc, char** argv)
{
    char* host;
    int opt;
    unsigned short portNumber;
    unsigned messageLength = 16, threadCount = 5;
    struct hostent* hp;
    pthread_t* threadList;
    iteration = 5;

    while((opt = getopt(argc, argv, OPTIONS)) != -1 )
    {
        switch(opt)
        {
            case 's':
                host = optarg;
                break;

            case 'p':
                portNumber = atoi(optarg);
                break;

            case 'i':
                iteration = atoi(optarg);
                break;

            case 'l':
                messageLength = atoi(optarg);
                break;

            case 't':
                threadCount = atoi(optarg);
                break;
        }
    }

    memset(&server, sizeof(struct sockaddr_in), 0);
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    if((hp = gethostbyname(host)) == NULL)
    {
        fprintf(stderr, "Invalid server\n");
        exit(1);
    }
    memcpy(&server.sin_addr, hp->h_addr, hp->h_length);

    threadList = malloc(sizeof(pthread_t) * threadCount );
    for(int i = 0; i < threadCount; ++i)
    {
        pthread_create(&threadList[i], NULL, clientThread, NULL);
    }

    for(int i = 0; i < threadCount; ++i)
    {
        pthread_join(threadList[i], NULL);
    }
    free(threadList);
    return 0;
}

void* clientThread(void* threadArg)
{
    FILE* threadFile;
    char fileName[32];
    pthread_t id = pthread_self();
    sprintf(fileName, "Thread%lu.txt", (unsigned long)id);
    threadFile = fopen(fileName, "w");
    fprintf(threadFile,"testing\n");
    fclose(threadFile);
    return 0;
}