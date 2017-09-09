#include "packet.h"
#include "stringutils.h"
#include "debugutils.h"
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <error.h>
#include <assert.h>

void fill_packet(struct packet* pkt, u_short flag, u_short seq_id, u_short offset, u_char *payload){
	fill_header(pkt, flag, seq_id, offset);
  memset(pkt->payload, 0, sizeof(pkt->payload));
	fill_payload(pkt, payload);
}
void fill_header(struct packet* pkt, u_short flag, u_short seq_id, u_short offset){
  bzero(pkt, sizeof(struct packet));
  pkt->hdr.offset = offset;
  pkt->hdr.seq_id = seq_id;
  pkt->hdr.flag = flag;
}

void fill_payload(struct packet* pkt, u_char *payload){
  bzero(pkt->payload, sizeof(pkt->payload));
  if (pkt->hdr.flag == ACK) { 
    // Do Nothing
  } else if (pkt->hdr.flag == READ) {
    // Sending file name in the packet; only for seq_id == 0
    if (pkt->hdr.seq_id == 0) {
      strcpy(pkt->payload, payload);
      pkt->hdr.offset = (u_short) strlen(payload); // get filename.txt 4:"get " 
    }
  } else if (pkt->hdr.flag == WRITE) {
      memcpy(pkt->payload, payload, pkt->hdr.offset);
  } else if (pkt->hdr.flag == 3) {
  } else if (pkt->hdr.flag == 4) {
  } else if (pkt->hdr.flag == 5) {
  } else if (pkt->hdr.flag == 6) {
  } 
}

void getfilenamefrompkt(u_char *buff, struct packet * pkt){
  memcpy(buff, pkt->payload, pkt->hdr.offset);
}

ssize_t sendpkt(int sock, struct packet *pkt, u_short flag, u_short seq_id, u_short offset, u_char * payload,  struct sockaddr_in *remote_r, socklen_t remote_length){
  
  fill_packet(pkt, flag, seq_id, offset, payload);
  return sendwithsock(sock, pkt, remote_r, remote_length);

}

ssize_t waitforpkt(int sock, struct packet *prev_pkt, struct packet *pkt, struct sockaddr_in *remote, socklen_t remote_length){
  // TODO: Implement check for the packet received to be the one we expect
  ssize_t nbytes;
  while(1) {
    nbytes = recvwithsock(sock, pkt, remote, &remote_length);
    // TODO: Check if the correct packet is received
    if(nbytes == PACKET_SIZE){ 
      break;
    }
    else {
      // For the case when socket timesout
      DEBUGS1("Socket timeout Sending Again");
      nbytes = sendwithsock(sock, prev_pkt, remote, remote_length);
    }
  }
  return nbytes;
}


ssize_t sendwithsock(int sock, struct packet* pkt, struct sockaddr_in* remote, socklen_t remote_length) {
  // TODO: Surety that this command is sent as a single chunk

  ssize_t nbytes = sendto(sock, pkt, PACKET_SIZE, 0, (struct sockaddr *)remote, remote_length);
  return nbytes;
}

ssize_t recvwithsock(int sock, struct packet* pkt, struct sockaddr_in* from_addr, socklen_t* from_addr_length_r) {
  // recvfrom stores the information of sender in from_addr
  // This will keep on blocking in case the messages are lost while in flight

  ssize_t nbytes = recvfrom(sock, pkt, PACKET_SIZE, 0, (struct sockaddr *)from_addr, from_addr_length_r);
  return nbytes;
}


void chunkreadfromsocket(int sock, struct packet *sent_pkt, struct packet *recv_pkt, u_char *file_name, struct sockaddr_in *remote, socklen_t remote_length){
  u_short seq_id;
  ssize_t nbytes;
  FILE *fp;
  fp = fopen(file_name, "wb");

  while(1){
    // Waiting for next Data Packet
    nbytes = waitforpkt(sock, sent_pkt, recv_pkt, remote, remote_length);
    
    DEBUGN("\t\tData Packet Recv", recv_pkt->hdr.seq_id);
    debug_print_pkt(recv_pkt);

    fwrite(recv_pkt->payload, sizeof(u_char), recv_pkt->hdr.offset, fp);

    seq_id = recv_pkt->hdr.seq_id; 
    
    // Packet sent is ACK for Data Packet Recv 
    nbytes = sendpkt(sock, sent_pkt, ACK, seq_id, 0, NULL, remote, remote_length);
    
    DEBUGN("\t\tACK Packet Sent", seq_id);
    debug_print_pkt(sent_pkt);

    if (recv_pkt->hdr.offset < PAYLOAD_SIZE) break;
  }

  // Server should keep on sending the last packet
  fclose(fp);
}

void chunkwritetosocket(int sock, struct packet *sent_pkt, struct packet *recv_pkt, u_char *file_name, struct sockaddr_in *remote, socklen_t remote_length) {
  
  ssize_t nbytes;
  u_short offset, seq_id;
  FILE * fp;
  u_char payload_buffer[PAYLOAD_SIZE];

  DEBUGS1("Reading from file and writing to socket");
  fp = fopen(file_name, "rb");
  while( ( offset = fread(payload_buffer, sizeof(u_char), PAYLOAD_SIZE, fp) ) != 0){

    seq_id = recv_pkt->hdr.seq_id + 1;

    // Send data packets
    nbytes = sendpkt(sock, sent_pkt, WRITE, seq_id, offset, payload_buffer, remote, remote_length);

    DEBUGN("\t\tData Packet Sent", sent_pkt->hdr.seq_id); 
    debug_print_pkt(sent_pkt);
    
    // Get ACK packets
    nbytes = waitforpkt(sock, sent_pkt, recv_pkt, remote, remote_length);

    DEBUGN("\t\tACK Packet Recv", recv_pkt->hdr.seq_id);
    debug_print_pkt(recv_pkt);
  }

  fclose(fp);

}
