#Makefile
CC = gcc
INCLUDE = /usr/lib
LIBS =
OBJS = 
CFLAGS = -g
all: server

server:
	$(CC) -o server ../headers/*.c  server.c $(CFLAGS) $(LIBS)

clean:
	rm -r server
