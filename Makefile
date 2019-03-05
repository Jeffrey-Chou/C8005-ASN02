CC=gcc
CFLAGS=-Wall -ggdb -lpthread

client: Client.c
	$(CC) $(CFLAGS) Client.c -o client

select: SelectServer.c
	$(CC) $(CFLAGS) SelectServer.c -o select

epoll: EpollServer.c
	$(CC) $(CFLAGS) EpollServer.c -o epoll

clean:
	rm -f client select epoll *.txt
