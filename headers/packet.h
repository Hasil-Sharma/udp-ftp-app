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

typedef unsigned char schar;
struct header {
  uint16_t seq_id; // id of the packet sent TODO: what if number of packets overflow ?
  uint16_t offset; // number of bytes to read from packet payload offset < PAYLOAD_SIZE means last packet and operation is completed
  uint16_t flag;
  /* To specify the type of packet it is
   * flag = 0 : ACK Packet
     flag = 1 : Read Packet
     flag = 2 : Write/Data Packet
     flag = 3 : Delete Packet
     flag = 4 : List Packet
     flag = 5 : Exit Packet
     flag = 6 : Unknow Packet
   */
  uint16_t checksum; 
};

struct packet {
  struct header hdr;
  schar payload[PAYLOAD_SIZE] ;
};

void fill_packet(struct packet*, uint16_t, uint16_t, uint16_t, schar *);

void fill_header(struct packet*, uint16_t, uint16_t, uint16_t);

void fill_payload(struct packet*, schar *);

void getfilenamefrompkt(schar *, struct packet *);

int sendpkt(int, void*, uint16_t, uint16_t, uint16_t, schar*,  struct sockaddr_in*, unsigned int);

int waitforpkt(int, void*, void *, struct sockaddr_in *, unsigned int*, struct sockaddr_in *, unsigned int);

int sendwithsock(int, void*, struct sockaddr_in*, unsigned int); 

int recvwithsock(int, void*, struct sockaddr_in*, unsigned int*);

void chunkreadfromsocket(int, struct packet*, struct packet*, uint16_t, uint16_t, uint16_t, schar*, struct sockaddr_in*, unsigned int, struct sockaddr_in*, unsigned int *); 

void chunkwritetosocket(int, struct packet*, struct packet*, uint16_t, schar*, struct sockaddr_in*, unsigned int);

#endif
