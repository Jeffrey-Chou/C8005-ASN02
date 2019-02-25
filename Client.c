#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_PORT 7000
#define OPTIONS "?s:p:i:l:t:"

int main(int argc, char** argv)
{
    char* host;
    int opt;
    unsigned short portNumber;
    unsigned iteration, messageLength, processCount;
    struct sockaddr_in server;
    struct hostent* hp;

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
                processCount = atoi(optarg);
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

    
    return 0;
}