#Makefile
CC = gcc
INCLUDE = /usr/lib
LIBS =
OBJS = 
CFLAGS = -g
all: client

client:
	$(CC) -o client ../headers/*.c client.c $(CFLAGS) $(LIBS)

clean:
	rm -r client
