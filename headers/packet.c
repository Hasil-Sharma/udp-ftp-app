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

void getstringfrompayload(u_char *buff, struct packet * pkt){
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
    //

    /*Cases:
     * - Case-1: (Server) prev_pkt: Nothing recv_pkt: (READ_RQ, WRITE_RQ, LS_RQ, DL_RQ or EXIT_RQ) Only when no operation is running
     * - Case-2: (Client) prev_pkt: (READ_RQ, WRITE_RQ, LS_RQ, DL_RQ or EXIT_RQ) recv_pkt: (READ_RQ, LS_RQ): Data Pkt & (WRITE_RQ, EXIT_RQ, DL_RQ): ACK Pkt
     * - Case-3: (Client/Server) prev_pkt: Data pkt recv_pkt: ACK with same seq_id
     * - Case-4: (Client/Server) prev_pkt: ACK recv_pkt: Data pkt with ACK:seq_id + 1
     * - Case-5: (Client/Server) prev_pkt: ACK recv_pkt: Data pkt with ACK:seq_id (ACK was lost or delayed)
     * - Case-6: (Client/Server) prev_pkt: Nothing recv_pkt: Data (case of last ACK sent not received, resend the ACK)
     * */
    if(nbytes == PACKET_SIZE){ 
      
      /*// PACKET_SIZE is correct*/
      // TODO: Deliberate More

      if( checkpktflag(prev_pkt, NO_FLAG) && checkreqflags(recv_pkt) ) break; // Case-1

      if( checkpktwithwriteresponse(prev_pkt) && checkpktflag(recv_pkt, WRITE))
        if(getpktseqid(recv_pkt) == getpktseqid(prev_pkt) + 1 ) break; // Case-2, 4
        if(checkpktflag(prev_pkt, ACK) && getpktseqid(recv_pkt) == getpktseqid(prev_pkt)) nbytes = sendwithsock(sock, prev_pkt, remote, remote_length); // Case-5

      if( checkpkwithackresponse(prev_pkt) && checkpktflag(recv_pkt, ACK))
        if(getpktseqid(prev_pkt) == getpktseqid(recv_pkt)) break;


      if( checkpktflag(prev_pkt, WRITE) && checkpktflag(recv_pkt, ACK) && getpktseqid(recv_pkt) == getpktseqid(prev_pkt) ) break; // case-3

      if( checkpktflag(prev_pkt, NO_FLAG) && checkpktflag(recv_pkt, WRITE) ) { // Case-6

        nbytes = sendpkt(sock, prev_pkt, ACK, getpktseqid(recv_pkt), 0, NULL, remote, remote_length); 
        bzero(prev_pkt, sizeof(prev_pkt));

      }

    } else if (nbytes < PACKET_SIZE && nbytes > 0) {

      DEBUGS1("partial packet received");

    } else {

      if (prev_pkt->hdr.flag == LS_RQ) break;
      // for the case when socket timesout
      DEBUGS1("socket timeout sending again");
      nbytes = sendwithsock(sock, prev_pkt, remote, remote_length);

    }
  }

  if (timeout_flag) unsetsocktimeout(sock);
  return nbytes;
}


ssize_t sendwithsock(int sock, struct packet* pkt, struct sockaddr_in* remote, socklen_t remote_length) {
  // TODO: surety that this command is sent as a single chunk

  ssize_t nbytes = sendto(sock, pkt, PACKET_SIZE, 0, (struct sockaddr *)remote, remote_length);
  return nbytes;
}


ssize_t recvwithsock(int sock, struct packet* pkt, struct sockaddr_in* from_addr, socklen_t* from_addr_length_r) {
  // recvfrom stores the information of sender in from_addr
  // this will keep on blocking in case the messages are lost while in flight

  ssize_t nbytes = recvfrom(sock, pkt, PACKET_SIZE, 0, (struct sockaddr *)from_addr, from_addr_length_r);
  return nbytes;
}


void chunkreadfromsocket(int sock, struct packet *sent_pkt, struct packet *recv_pkt, u_char *file_name, struct sockaddr_in *remote, socklen_t remote_length){
  
  /* get client
   * initially: 
   * - sent_pkt: read req data packet
   * - recv_pkt: first data packet
   * */

  /* put response server
   * intially:
   * - sent_pkt: ack to write req packet
   * - recv_pkt: write req packet
   * */

  u_short seq_id;
  ssize_t nbytes;
  FILE *fp;
  fp = fopen(file_name, "wb");

  while(TRUE){

    /* get client
     *
     * initially :
     * - waiting for data packet in reply to read req
     * - resend read req is not data packet is received
     *
     * later :
     * - waiting for data packet in reply to ack sent
     * - !(resend ack if no data packet is received)
     * */

    /* put response server
     * - waiting for data packet in reply to ack sent
     * - !(resend ack if no data packet is received)
     * */

    // writing the first data packet received
    fwrite(recv_pkt->payload, sizeof(u_char), recv_pkt->hdr.offset, fp);

    seq_id = recv_pkt->hdr.seq_id; 
    
    // send ack packet for data packet recv (both have same seq id) 
    nbytes = sendpkt(sock, sent_pkt, ACK, seq_id, 0, NULL, remote, remote_length);
    
    DEBUGN("\t\tack packet sent", seq_id);
    debug_print_pkt(sent_pkt);

    if (recv_pkt->hdr.offset < PAYLOAD_SIZE) break;

    // wait for next data packet - without timeout
    // do not resend the ack if no data packet received
    nbytes = waitforpkt(sock, sent_pkt, recv_pkt, remote, remote_length, FALSE);

    DEBUGN("\t\tdata packet recv", recv_pkt->hdr.seq_id);
    debug_print_pkt(recv_pkt);

  }

  // server should keep on sending the last packet
  fclose(fp);
}

void chunkwritetosocket(int sock, struct packet *sent_pkt, struct packet *recv_pkt, u_char *file_name, struct sockaddr_in *remote, socklen_t remote_length) {

  /* put client
   *
   * initially:
   * - sent_pkt : zero filled packet
   * - recv_pkt : ack for write packet sent
   * */  

  /* get response server
   *
   * initially:
   * - sent_pkt : zero filled packet
   * - recv_pkt : read req
   * */

  ssize_t nbytes;
  u_short offset, seq_id;
  FILE * fp;
  u_char payload_buffer[PAYLOAD_SIZE];
  
  DEBUGS1("reading from file and writing to socket");
  fp = fopen(file_name, "rb");

  if (!fp) perror(file_name);
  
  while( ( offset = fread(payload_buffer, sizeof(u_char), PAYLOAD_SIZE, fp) ) != 0){
    
    /* put client
     * - reply ack:seq_id with data packet: seq_id + 1 
     * - resend data packet: seq_id + 1 if ack: seq_id + 1 isn't received 
     * */

    /* get response server
     *
     * initially:
     * - reply read:seq_id with data packet: seq_id + 1
     * - resend data packet: seq_id + 1 if ack: seq_id + 1 isn't received
     *
     * later:
     * - reply ack:seq_id with data packet: ack:seq_id +1
     * - resend data packet: ack:seq_id + 1 if ack: seq_id + 1 isn't received
     * */

    seq_id = getpktseqid(recv_pkt) + 1;

    // send data packet with seq_id =  ack:seq_id + 1
    nbytes = sendpkt(sock, sent_pkt, WRITE, seq_id, offset, payload_buffer, remote, remote_length);

    DEBUGN("\t\tdata packet sent", sent_pkt->hdr.seq_id); 
    debug_print_pkt(sent_pkt);
    
    // get ack packet for data packet: seq_id or resend data packet: seq_id 
    nbytes = waitforpkt(sock, sent_pkt, recv_pkt, remote, remote_length, TRUE);

    DEBUGN("\t\tack packet recv", recv_pkt->hdr.seq_id);
    debug_print_pkt(recv_pkt);
  }


  fclose(fp);

}

void setsocktimeout(int sock){
  // adding timeout
  struct timeval tv;
  tv.tv_sec = 5; // 5 sec time out 
  tv.tv_usec = 0;  // not init'ing this can cause strange errors
  if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval)) != 0)
    perror("unable to set socket timeout");

}

void unsetsocktimeout(int sock){

  struct timeval tv;
  tv.tv_sec = 0; // 5 sec time out 
  tv.tv_usec = 0;  // not init'ing this can cause strange errors
  if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval)) != 0)
    perror("unable to unset socket timeout");

}

u_short getpktseqid(struct packet* pkt){
  return pkt->hdr.seq_id;
}

int checkreqflags(struct packet* pkt) {
  return checkpktflag(pkt, READ_RQ) || checkpktflag(pkt, WRITE_RQ) || checkpktflag(pkt, LS_RQ) || checkpktflag(pkt, EXIT_RQ) || checkpktflag(pkt, DL_RQ);
}

int checkpktflag(struct packet* pkt, int req) {
  return pkt->hdr.flag == req;
}

int checkpkwithackresponse(struct packet* pkt){

  return checkpktflag(pkt, WRITE_RQ) || checkpktflag(pkt, EXIT_RQ) || checkpktflag(pkt, DL_RQ) || checkpktflag(pkt, WRITE);
}

int checkpktwithwriteresponse(struct packet* pkt){

  return checkpktflag(pkt, ACK) || checkpktflag(pkt, READ_RQ) || checkpktflag(pkt, LS_RQ);  

}
