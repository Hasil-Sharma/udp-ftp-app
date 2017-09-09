#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#ifndef HEADER_H
#define HEADER_H

#define PACKET_SIZE 512
#define HEADER_SIZE 8
#define PAYLOAD_SIZE (PACKET_SIZE - HEADER_SIZE)
#define ACK 0
#define READ 1
#define WRITE 2

typedef unsigned char u_char;
struct header {
  u_short seq_id; // id of the packet sent TODO: what if number of packets overflow ?
  u_short offset; // number of bytes to read from packet payload offset < PAYLOAD_SIZE means last packet and operation is completed
  u_short flag;
  /* To specify the type of packet it is
   * flag = 0 : ACK Packet
     flag = 1 : Read Packet
     flag = 2 : Write/Data Packet
     flag = 3 : Delete Packet
     flag = 4 : List Packet
     flag = 5 : Exit Packet
     flag = 6 : Unknow Packet
   */
  u_short checksum; 
};

struct packet {
  struct header hdr;
  u_char payload[PAYLOAD_SIZE] ;
};

void fill_packet(struct packet*, u_short, u_short, u_short, u_char *);

void fill_header(struct packet*, u_short, u_short, u_short);

void fill_payload(struct packet*, u_char *);

void getfilenamefrompkt(u_char *, struct packet *);

ssize_t sendpkt(int, struct packet*, u_short, u_short, u_short, u_char*, struct sockaddr_in*, socklen_t);

ssize_t waitforpkt(int, struct packet*, struct packet *, struct sockaddr_in *, socklen_t);

ssize_t sendwithsock(int, struct packet*, struct sockaddr_in*, socklen_t); 

ssize_t recvwithsock(int, struct packet*, struct sockaddr_in*, socklen_t*);

void chunkreadfromsocket(int, struct packet*, struct packet*, u_char*, struct sockaddr_in*, socklen_t); 

void chunkwritetosocket(int, struct packet*, struct packet*, u_char*, struct sockaddr_in*, socklen_t);

#endif
