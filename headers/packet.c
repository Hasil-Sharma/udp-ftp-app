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
  } else if (pkt->hdr.flag == READ_RQ || pkt->hdr.flag == WRITE_RQ) {
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

ssize_t waitforpkt(int sock, struct packet *prev_pkt, struct packet *recv_pkt, struct sockaddr_in *remote, socklen_t remote_length, int timeout_flag){
  // TODO: Implement check for the packet received to be the one we expect
  ssize_t nbytes;

  if (timeout_flag) setsocktimeout(sock);

  while(TRUE) {
    nbytes = recvwithsock(sock, recv_pkt, remote, &remote_length);
    
    // TODO: Check if the correct packet is received
    
    /* Case-1: prev_pkt: READ Req pkt, recv_pkt: Data pkt 
     * Case-2: prev_pkt: ACK pkt, recv_pkt: Data Pkt 
     * Case-3: prev_pkt: Data/Write Req pkt, recv_pkt: ACK Pkt
     * Case-4: prev_pkt: NULL, recv_pkt: READ Req pkt
     * Case-5: prev_pkt: NULL, recv_pkt: WRITE Req pkt
     */

    if(nbytes == PACKET_SIZE){ 
      
      /*// PACKET_SIZE is correct*/
      if( prev_pkt->hdr.flag == NO_FLAG && (recv_pkt->hdr.flag == READ_RQ || recv_pkt->hdr.flag == WRITE_RQ) ) break; // Case-4,5

      if( (prev_pkt->hdr.flag == ACK || prev_pkt->hdr.flag == READ_RQ) && recv_pkt->hdr.flag == WRITE && prev_pkt->hdr.seq_id == recv_pkt->hdr.seq_id - 1 ) break; //Case-1,2

      if( (prev_pkt->hdr.flag == WRITE || prev_pkt->hdr.flag == WRITE_RQ) && recv_pkt->hdr.flag == ACK && prev_pkt->hdr.seq_id == recv_pkt->hdr.seq_id ) break;


    } else if (nbytes < PACKET_SIZE && nbytes > 0) {

      DEBUGS1("Partial Packet Received");

    } else {

      // For the case when socket timesout
      DEBUGS1("Socket timeout Sending Again");
      nbytes = sendwithsock(sock, prev_pkt, remote, remote_length);

    }
  }

  if (timeout_flag) unsetsocktimeout(sock);
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
  
  /* GET CLIENT
   * Initially: 
   * - sent_pkt: READ Req Data Packet
   * - recv_pkt: First Data Packet
   * */

  /* PUT RESPONSE SERVER
   * Intially:
   * - sent_pkt: ACK to WRITE Req Packet
   * - recv_pkt: WRITE Req Packet
   * */

  u_short seq_id;
  ssize_t nbytes;
  FILE *fp;
  fp = fopen(file_name, "wb");

  while(TRUE){

    /* GET CLIENT
     *
     * Initially :
     * - Waiting for Data Packet in reply to READ Req
     * - Resend READ Req is not Data Packet is received
     *
     * Later :
     * - Waiting for Data Packet in reply to ACK Sent
     * - !(Resend ACK if no Data Packet is received)
     * */

    /* PUT RESPONSE SERVER
     * - Waiting for Data Packet in reply to ACK Sent
     * - !(Resend ACK if no Data Packet is received)
     * */

    // Writing the first Data Packet Received
    fwrite(recv_pkt->payload, sizeof(u_char), recv_pkt->hdr.offset, fp);

    seq_id = recv_pkt->hdr.seq_id; 
    
    // Send ACK Packet for Data Packet Recv (both have same seq id) 
    nbytes = sendpkt(sock, sent_pkt, ACK, seq_id, 0, NULL, remote, remote_length);
    
    DEBUGN("\t\tACK Packet Sent", seq_id);
    debug_print_pkt(sent_pkt);

    if (recv_pkt->hdr.offset < PAYLOAD_SIZE) break;

    // Wait for next data packet - Without timeout
    // Do not resend the ACK if no data packet received
    nbytes = waitforpkt(sock, sent_pkt, recv_pkt, remote, remote_length, FALSE);

    DEBUGN("\t\tData Packet Recv", recv_pkt->hdr.seq_id);
    debug_print_pkt(recv_pkt);

  }

  // Server should keep on sending the last packet
  fclose(fp);
}

void chunkwritetosocket(int sock, struct packet *sent_pkt, struct packet *recv_pkt, u_char *file_name, struct sockaddr_in *remote, socklen_t remote_length) {

  /* PUT CLIENT
   *
   * Initially:
   * - sent_pkt : zero filled packet
   * - recv_pkt : ACK for WRITE Packet sent
   * */  

  /* GET RESPONSE SERVER
   *
   * Initially:
   * - sent_pkt : zero filled packet
   * - recv_pkt : READ Req
   * */

  ssize_t nbytes;
  u_short offset, seq_id;
  FILE * fp;
  u_char payload_buffer[PAYLOAD_SIZE];
  
  DEBUGS1("Reading from file and writing to socket");
  fp = fopen(file_name, "rb");

  if (!fp) perror(file_name);
  
  while( ( offset = fread(payload_buffer, sizeof(u_char), PAYLOAD_SIZE, fp) ) != 0){
    
    /* PUT CLIENT
     * - Reply ACK:seq_id with Data Packet: seq_id + 1 
     * - Resend Data Packet: seq_id + 1 if ACK: seq_id + 1 isn't received 
     * */

    /* GET RESPONSE SERVER
     *
     * Initially:
     * - Reply READ:seq_id with Data Packet: seq_id + 1
     * - Resend Data Packet: seq_id + 1 if ACK: seq_id + 1 isn't received
     *
     * Later:
     * - Reply ACK:seq_id with Data Packet: ACK:seq_id +1
     * - Resend Data Packet: ACK:seq_id + 1 if ACK: seq_id + 1 isn't received
     * */

    seq_id = recv_pkt->hdr.seq_id + 1;

    // Send Data Packet with seq_id =  ACK:seq_id + 1
    nbytes = sendpkt(sock, sent_pkt, WRITE, seq_id, offset, payload_buffer, remote, remote_length);

    DEBUGN("\t\tData Packet Sent", sent_pkt->hdr.seq_id); 
    debug_print_pkt(sent_pkt);
    
    // Get ACK packet for Data Packet: seq_id or resend Data Packet: seq_id 
    nbytes = waitforpkt(sock, sent_pkt, recv_pkt, remote, remote_length, TRUE);

    DEBUGN("\t\tACK Packet Recv", recv_pkt->hdr.seq_id);
    debug_print_pkt(recv_pkt);
  }


  fclose(fp);

}

void setsocktimeout(int sock){
  // Adding timeout
  struct timeval tv;
  tv.tv_sec = 5; // 5 sec time out 
  tv.tv_usec = 0;  // Not init'ing this can cause strange errors
  if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval)) != 0)
    perror("Unable to set socket timeout");

}

void unsetsocktimeout(int sock){

  struct timeval tv;
  tv.tv_sec = 0; // 5 sec time out 
  tv.tv_usec = 0;  // Not init'ing this can cause strange errors
  if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval)) != 0)
    perror("Unable to unset socket timeout");

}
