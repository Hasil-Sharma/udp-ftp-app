#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#ifndef HEADER_H
#define HEADER_H

#define TRUE 1
#define FALSE 0
#define PACKET_SIZE 1024
#define HEADER_SIZE 8
#define PAYLOAD_SIZE (PACKET_SIZE - HEADER_SIZE)

#define NO_FLAG 0
#define READ_RQ 1
#define WRITE_RQ 2
#define LS_RQ 3
#define DL_RQ 4
#define EXIT_RQ 5
#define UNK_RQ 6
#define ACK 7
#define WRITE 8

struct header {
  u_short seq_id; // id of the packet sent TODO: what if number of packets overflow ?
  u_short offset; // number of bytes to read from packet payload offset < PAYLOAD_SIZE means last packet and operation is completed
  u_short flag;
  u_short checksum; 
};

struct packet {
  struct header hdr;
  u_char payload[PAYLOAD_SIZE] ;
};

typedef struct packet packet;
typedef struct sockaddr_in sockaddr_in;

void fill_packet( packet*, u_short, u_short, u_short, u_char *);

void fill_header( packet*, u_short, u_short, u_short);

void fill_payload( packet*, u_char *);

void getstringfrompayload(u_char *,  packet *);

ssize_t sendpkt(int,  packet*, u_short, u_short, u_short, u_char*,  sockaddr_in*, socklen_t);

ssize_t waitforpkt(int,  packet*,  packet *,  sockaddr_in *, socklen_t, int);

ssize_t sendwithsock(int,  packet*,  sockaddr_in*, socklen_t); 

ssize_t recvwithsock(int,  packet*,  sockaddr_in*, socklen_t*);

void chunkreadfromsocket(int,  packet*,  packet*, char*,  sockaddr_in*, socklen_t); 

void chunkwritetosocket(int,  packet*,  packet*, char*,  sockaddr_in*, socklen_t);

void setsocktimeout(int);

void unsetsocktimeout(int);

u_short getpktseqid( packet*);

int checkreqflags( packet*);

int checkpktflag( packet*, int);

int checkpktwithwriteresponse( packet*);

int checkpkwithackresponse( packet*);

u_short getchecksum( packet* );

u_short cksum(u_short *, int);

int checkcksum( packet *);

void encdecpayload(u_char *, int offset);

#endif
