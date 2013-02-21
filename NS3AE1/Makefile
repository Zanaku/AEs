CC=gcc
CFLAGS=-W -Wall -Werror

web_server: web_server.o
	$(CC) -o $@ $^

%.o: %.c
	$(CC) -c $(CFLAGS) $<

clean:
	rm *.o;
