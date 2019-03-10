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
#include <sys/wait.h>

#define SERVER_PORT 8000
#define TASK_COUNT 100
#define OPTIONS "?s:i:l:t:"

void generateMessage();
int clientThread(int id);


struct sockaddr_in server;
unsigned iteration, messageLength;
char* message;
int error;

int main(int argc, char** argv)
{
    char* host;
    int opt, ret = 0, status;
    unsigned clientCount = 5;
    struct hostent* hp;
    //pthread_t* threadList;
    pid_t* processList, pid;
    iteration = 5, messageLength = 16;
    error = 0;

    while((opt = getopt(argc, argv, OPTIONS)) != -1 )
    {
        switch(opt)
        {
            case 's':
                host = optarg;
                break;

            case 'i':
                iteration = atoi(optarg);
                break;

            case 'l':
                messageLength = atoi(optarg);
                break;

            case 't':
                clientCount = atoi(optarg);
                break;
            default:
                printf("Valid arguments are -s -i - l -t\n");
                return 1;
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

    //threadList = malloc(sizeof(pthread_t) * clientCount );
    processList = malloc(sizeof(pid_t) * clientCount);
    for(int i = 0; i < clientCount; ++i)
    {
        //pthread_create(&threadList[i], NULL, clientThread, NULL);
        pid = fork();
        if(pid < 0)
        {
            return 1;
        }
        if(pid == 0)
        {
            ret = clientThread(i);
            return ret;
        }
        processList[i] = pid;
        
    }

    for(int i = 0; i < clientCount; ++i)
    {
        //pthread_join(threadList[i], NULL);
        waitpid(processList[i], &status, 0);
        if(status > 0)
        {
            error = 1;
        }
        
    }
    //free(threadList);
    free(processList);
    free(message);
    printf("Client done\n");
    if(error)
    {
        return 1;
    }
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
    printf("Send Message:%s\n", message);
}

int clientThread(int id)
{
    FILE* threadFile;
    char fileName[64];
    int sd;
    char* buffer = malloc(sizeof(char) * (messageLength + 1));
    //pthread_t id = pthread_self();
    char* bp;
    struct timeval start, end;
    double elapsedTime = 0, totalTime = 0;

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        return 1;
    }

    if ( connect(sd, (struct sockaddr*)&server, sizeof(struct sockaddr_in)) == -1)
    {
        return 1;
    }

    sprintf(fileName, "Client-%d.txt", id);
    threadFile = fopen(fileName, "w");
    fprintf(threadFile,"Sending message length %u to the server %u times\n\n", messageLength, iteration);
    
    for(unsigned i = 0; i < iteration; ++i)
    {
        int n = 0, bytesLeft = messageLength;
        
        unsigned length = htonl(messageLength);
        bp = (char*)&length;
        n = send(sd, bp, sizeof(length), 0);
        if(n == -1)
        {
            close(sd);
            fclose(threadFile);
            error = 1;
            return 1;
        }
        n = send(sd, message, messageLength, 0);
        if(n == -1)
        {
            close(sd);
            fclose(threadFile);
            error = 1;
            return 1;
        }
        gettimeofday(&start, NULL);
        bp = buffer;
        while((n = recv(sd,bp,bytesLeft, 0 )) < messageLength)
        {
            bp += n;
            bytesLeft -= n;
        }
        gettimeofday(&end, NULL);
        elapsedTime = (end.tv_sec - start.tv_sec) * 1000;
	    elapsedTime += (end.tv_usec - start.tv_usec) / 1000;
	    fprintf(threadFile, "Iteration %d: Time elapsed: %f msec\n", i, elapsedTime);
        if(n == -1)
        {
            close(sd);
            fclose(threadFile);
            return 1;
        }
        totalTime += elapsedTime;
    }
    fprintf(threadFile, "Average time per message: %f msec\n", totalTime/iteration);
    close(sd);
    fclose(threadFile);
    free(buffer);
    return 0;
}