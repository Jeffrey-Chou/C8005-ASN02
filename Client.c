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

#include <sys/time.h>

#define SERVER_PORT 8000
#define OPTIONS "?s:p:i:l:t:"

void generateMessage();
void* clientThread(void* threadArg);


struct sockaddr_in server;
unsigned iteration, messageLength;
char* message;

int main(int argc, char** argv)
{
    char* host;
    int opt;
    unsigned short portNumber;
    unsigned threadCount = 5;
    struct hostent* hp;
    pthread_t* threadList;
    iteration = 5, messageLength = 16;

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
    generateMessage();
    memset(&server, 0, sizeof(struct sockaddr_in));
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
    free(message);
    return 0;
}

void generateMessage()
{
    message = malloc(sizeof(char) * messageLength);
    for(unsigned i = 0; i < messageLength - 1; ++i)
    {
        message[i] = (i % 10) + '0';
    }
    message[messageLength - 1] = '\0';
    printf("%s\n", message);
}

void* clientThread(void* threadArg)
{
    FILE* threadFile;
    char fileName[32];
    int sd;
    char* buffer = malloc(sizeof(char) * messageLength);
    pthread_t id = pthread_self();
    struct timeval start, end;
    double elapsedTime = 0, totalTime = 0;

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        return 0;
    }

    if ( connect(sd, (struct sockaddr*)&server, sizeof(struct sockaddr_in)) == -1)
    {
        return 0;
    }

    sprintf(fileName, "Thread%lu.txt", (unsigned long)id);
    threadFile = fopen(fileName, "w");
    fprintf(threadFile,"Sending message length %u to the server %u times\n\n", messageLength, iteration);
    
    for(unsigned i = 0; i < iteration; ++i)
    {
        int n = 0, bytesLeft = messageLength;
        char* bp = buffer;
        unsigned length = htonl(messageLength);
        send(sd, &length, sizeof(length), 0);
        send(sd, message, messageLength, 0);
        gettimeofday(&start, NULL);
        while((n = recv(sd,bp,bytesLeft, 0 )) < messageLength)
        {
            bp += n;
            bytesLeft -= n;
        }
        gettimeofday(&end, NULL);
        elapsedTime = (end.tv_sec - start.tv_sec) * 1000;
	    elapsedTime += (end.tv_usec - start.tv_usec) / 1000;
	    fprintf(threadFile, "Iteration %d: Time elapsed: %f msec\n", i, elapsedTime);
        totalTime += elapsedTime;
    }
    fprintf(threadFile, "Average time per message: %f msec\n", totalTime/iteration);
    printf("connection done\n");
    close(sd);
    fclose(threadFile);
    return 0;
}