#include "packet.h"
#include "stringutils.h"
#include "debugutils.h"
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <error.h>

void fill_packet(struct packet* pkt, uint16_t flag, uint16_t seq_id, uint16_t offset, schar *payload){
	fill_header(pkt, flag, seq_id, offset);
	fill_payload(pkt, payload);
}
void fill_header(struct packet* pkt, uint16_t flag, uint16_t seq_id, uint16_t offset){
  bzero(pkt, sizeof(struct packet));
  pkt->hdr.offset = offset;
  pkt->hdr.seq_id = seq_id;
  pkt->hdr.flag = flag;
}

void fill_payload(struct packet* pkt, schar *payload){
  bzero(pkt->payload, sizeof(pkt->payload));
  if (pkt->hdr.flag == ACK) { 
    // Do Nothing
  } else if (pkt->hdr.flag == READ) {
    // Sending file name in the packet; only for seq_id == 0
    if (pkt->hdr.seq_id == 0) {
      strcpy(pkt->payload, payload);
      pkt->hdr.offset = (uint16_t) strlen(payload); // get filename.txt 4:"get " 
    }
  } else if (pkt->hdr.flag == WRITE) {
      memcpy(pkt->payload, payload, pkt->hdr.offset);
  } else if (pkt->hdr.flag == 3) {
  } else if (pkt->hdr.flag == 4) {
  } else if (pkt->hdr.flag == 5) {
  } else if (pkt->hdr.flag == 6) {
  } 
}

void getfilenamefrompkt(schar *buff, struct packet * pkt){
  strncpy(buff, pkt->payload, pkt->hdr.offset);
}

int sendpkt(int sock, void *pkt, uint16_t flag, uint16_t seq_id, uint16_t offset, schar * payload,  struct sockaddr_in *remote_r, unsigned int remote_length){
  
  fill_packet(pkt, flag, seq_id, offset, payload);
  return sendwithsock(sock, pkt, remote_r, remote_length);

}

int waitforpkt(int sock, void *prev_pkt,void *pkt, struct sockaddr_in *from_addr, unsigned int * from_addr_length_r, struct sockaddr_in *remote, unsigned int remote_length){
  // TODO: Implement check for the packet received to be the one we expect
  int nbytes;
  DEBUGS1("Wait for Pkt Method : Start");
  while(1) {
    nbytes = recvwithsock(sock, pkt, from_addr, from_addr_length_r);
    
    if(nbytes == PACKET_SIZE){ 
      DEBUGN("Bytes Received", nbytes);
      break;
    }
    /*else if(nbytes == EAGAIN || nbytes == EWOULDBLOCK)*/
    else {
      DEBUGS1("No Response Tramsmitting again");
      nbytes = sendwithsock(sock, prev_pkt, remote, remote_length);
      DEBUGN("Bytes Sent", nbytes);
    }
  }
  DEBUGS1("Wait for Pkt Method : End");
  return nbytes;
}
int sendwithsock(int sock, void *pkt, struct sockaddr_in *remote, unsigned int remote_length) {
  // TODO: Surety that this command is sent as a single chunk
  return sendto(sock, pkt, PACKET_SIZE, 0, (struct sockaddr *)remote, remote_length);
}

int recvwithsock(int sock, void *pkt, struct sockaddr_in * from_addr, unsigned int * from_addr_length_r) {
  // recvfrom stores the information of sender in from_addr
  // This will keep on blocking in case the messages are lost while in flight
  return recvfrom(sock, pkt, PACKET_SIZE, 0, (struct sockaddr *)from_addr, from_addr_length_r);
}
void chunkreadfromsocket(int sock, struct packet *send_pkt, struct packet *recv_pkt, uint16_t flag, uint16_t seq_id, uint16_t offset, schar *payload, struct sockaddr_in *remote, unsigned int remote_length, struct sockaddr_in *from_addr, unsigned int * from_addr_length_r){

  int nbytes;
  FILE *fp;

  nbytes = sendpkt(sock, send_pkt, READ, seq_id++, offset, payload, remote, remote_length);
  nbytes = waitforpkt(sock, send_pkt, recv_pkt, from_addr, from_addr_length_r, remote, remote_length);

  fp = fopen(payload, "wb");

  while(1){
    fwrite(recv_pkt->payload, sizeof(schar), recv_pkt->hdr.offset, fp);
    if (recv_pkt->hdr.offset < PAYLOAD_SIZE) break;
    nbytes = sendpkt(sock, send_pkt, ACK, seq_id++, 0, NULL, remote, remote_length);
    nbytes = waitforpkt(sock, send_pkt, recv_pkt, from_addr, from_addr_length_r, remote, remote_length);
  }

  // Server should keep on sending the last packet
  fclose(fp);
}

void chunkwritetosocket(int sock, struct packet *send_pkt, struct packet *recv_pkt, uint16_t flag, schar *payload, struct sockaddr_in * remote, unsigned int remote_length) {
  int nbytes;
  uint16_t offset;
  FILE * fp;
  schar buff[PAYLOAD_SIZE];

  fp = fopen(payload, "rb");
  while( ( offset = fread(buff, sizeof(schar), PAYLOAD_SIZE, fp) ) != 0){
  
    nbytes = sendpkt(sock, send_pkt, WRITE, recv_pkt->hdr.seq_id + 1, offset, buff, remote, remote_length);
    nbytes = waitforpkt(sock, send_pkt, recv_pkt, remote, &remote_length, remote, remote_length);
  }

  fclose(fp);

}
