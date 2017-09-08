#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifndef HEADER_H
#define HEADER_H

#define PACKET_SIZE 512
#define HEADER_SIZE 8
#define PAYLOAD_SIZE (PACKET_SIZE - HEADER_SIZE)
#define READ 1
#define ACK 0

struct header {
  unsigned short seq_id; // id of the packet sent TODO: what if number of packets overflow ?
  unsigned short offset; // number of bytes to read from packet payload offset < PAYLOAD_SIZE means last packet and operation is completed
  unsigned short flag;
  /* To specify the type of packet it is
   * flag = 0 : ACK Packet
     flag = 1 : Read Packet
     flag = 2 : Write/Data Packet
     flag = 3 : Delete Packet
     flag = 4 : List Packet
     flag = 5 : Exit Packet
     flag = 6 : Unknow Packet
   */
  unsigned short checksum; 
};

struct packet {
  struct header hdr;
  char payload[PAYLOAD_SIZE] ;
};

void fill_packet(struct packet*, short int, short int, char *);
void fill_header(struct packet*, short int, short int);
void fill_payload(struct packet*, char *);


int sendpkt(int, void*, int, unsigned int, char*,  struct sockaddr_in*, unsigned int);
int waitforpkt(int, void*, void *, struct sockaddr_in *, unsigned int*, struct sockaddr_in *, unsigned int);
int sendwithsock(int, void*, struct sockaddr_in*, unsigned int); 
int recvwithsock(int, void*, struct sockaddr_in*, unsigned int*);

#endif
