CC=gcc
CFLAGS=-W -Wall -Werror -g

web_server: web_server.o
	$(CC) -o $@ $^ -lpthread

%.o: %.c
	$(CC) -c $(CFLAGS) $<

clean:
	rm *.o;
