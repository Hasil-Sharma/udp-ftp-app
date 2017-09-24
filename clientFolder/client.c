#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

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

#define MAXBUFSIZE 120 // Set the limit on the length of the file

#define DEBUGS1(s) fprintf(stderr, "DEBUG: %s\n", s)
#define DEBUGSX(s) fprintf(stderr, "DEBUG: %u\n", s)
#define DEBUGN(d, s) fprintf(stderr,"DEBUG: %s: %d\n",d,s)
#define DEBUGS(d, s) fprintf(stderr,"DEBUG: %s: %s\n",d, s)
#define INFON(d, s) fprintf(stdout, "INFO: %s: %d\n", d, s)
#define INFOS(d, s) fprintf(stdout, "INFO: %s: %s\n", d, s)


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

void debug_print_pkt(struct packet *);
void debug_print_hdr(struct header *);

char * get_second_string(char * );

int main(int argc, char *argv[]){

  int sock, read_line;
  ssize_t nbytes;

  char buff[MAXBUFSIZE], file_name[MAXBUFSIZE];
  socklen_t remote_length, from_addr_length;
  u_short seq_id;
  struct sockaddr_in remote, from_addr;
  struct packet sent_pkt, recv_pkt;
  FILE * fp;
  int flag = TRUE, flag_connection = TRUE;

  if (argc < 3) {
    fprintf(stderr, "USAGE: <server_ip> <server_port>\n");
    exit(1);
  }

  bzero(&remote, sizeof(remote));

  remote.sin_family = AF_INET;
  remote.sin_port = htons(atoi(argv[2]));
  remote.sin_addr.s_addr = inet_addr(argv[1]); // TODO: Check for the error

  remote_length = sizeof(remote); // TODO: Test if changing it before assigment makes a difference
  from_addr_length = sizeof(from_addr);

  if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { fprintf(stderr, "unable to create socket\n"); }


  while(flag){

      bzero(buff, sizeof(buff));
      bzero(&sent_pkt, sizeof(sent_pkt));
      bzero(&recv_pkt, sizeof(recv_pkt));
      bzero(file_name, sizeof(file_name));

      fprintf(stdout, ">>>");
      if( fgets(buff, MAXBUFSIZE, stdin) != NULL && flag_connection) {

        // This does not breaks up the space delimited sentence

        buff[strlen(buff) - 1] = '\0'; // remove the trailing \n

        if (strncasecmp(buff, "get ", 4) == 0){

          seq_id = 0;

          strcpy(file_name, get_second_string(buff));
          // Sending READ Packet

          DEBUGS1("\t\tREAD Packet Sent");
          nbytes = sendpkt(sock, &sent_pkt, READ_RQ, seq_id, strlen(file_name), file_name, &remote, remote_length);
          debug_print_pkt(&sent_pkt);

          // Waiting for READ Req Response: Data Packet
          // Resend READ Req if no Data Packet

          nbytes = waitforpkt(sock, &sent_pkt, &recv_pkt, &remote, remote_length, TRUE);
          chunkreadfromsocket(sock, &sent_pkt, &recv_pkt, file_name, &remote, remote_length);

        } else if (strncasecmp(buff, "put ", 4) == 0){

          seq_id = 0;
          strcpy(file_name, get_second_string(buff));
          DEBUGS1(file_name);
          // Sending WRITE Packet

          DEBUGS1("\t\tWRITE Packet Sent");
          nbytes = sendpkt(sock, &sent_pkt, WRITE_RQ, seq_id, strlen(file_name), file_name, &remote, remote_length);

          debug_print_pkt(&sent_pkt);

          // Waiting for ACK for the WRITE Packet
          // Resend WRITE Req if no ACK Packet received

          nbytes = waitforpkt(sock, &sent_pkt, &recv_pkt, &remote, remote_length, TRUE);

          DEBUGS1("\t\tWRITE ACK Packet Received");
          debug_print_pkt(&recv_pkt);

          chunkwritetosocket(sock, &sent_pkt, &recv_pkt, file_name, &remote, remote_length);

        } else if (strncasecmp(buff, "delete ", 7) == 0){

          seq_id = 0;
          strcpy(file_name, get_second_string(buff));
          nbytes = sendpkt(sock, &sent_pkt, DL_RQ, seq_id, strlen(file_name), file_name, &remote, remote_length);

          DEBUGS1("\t\tDELETE packet sent");
          debug_print_pkt(&sent_pkt);

          nbytes = waitforpkt(sock, &sent_pkt, &recv_pkt, &remote, remote_length, TRUE);
          DEBUGS1("\t\tFile DELETED");

        } else if (strcasecmp(buff, "ls") == 0){

          seq_id = 0;
          nbytes = sendpkt(sock, &sent_pkt, LS_RQ, seq_id, 0, NULL, &remote, remote_length);

          debug_print_pkt(&sent_pkt);
          DEBUGS1("\t\tLS packet Sent");
          debug_print_pkt(&sent_pkt);

          // Waiting for WRITE Packet

          nbytes = waitforpkt(sock, &sent_pkt, &recv_pkt, &remote, remote_length, TRUE);
          bzero(buff, sizeof(buff));

          getstringfrompayload(buff, &recv_pkt);
          fprintf(stdout, "%s\n", buff);

        } else if (strcasecmp(buff, "exit") == 0){

          seq_id = 0;
          nbytes = sendpkt(sock, &sent_pkt, EXIT_RQ, seq_id, 0, NULL, &remote, remote_length);
          nbytes = waitforpkt(sock, &sent_pkt, &recv_pkt, &remote, remote_length, TRUE);
          flag_connection = FALSE;

        } else {

          seq_id = 0;

          nbytes = sendpkt(sock, &sent_pkt, UNK_RQ, seq_id, strlen(buff), buff, &remote, remote_length);

          DEBUGS1("\t\tUnknown packet Sent");
          debug_print_pkt(&sent_pkt);
          DEBUGS1(buff);
          nbytes = waitforpkt(sock, &sent_pkt, &recv_pkt, &remote, remote_length, TRUE);
          debug_print_pkt(&recv_pkt);
          bzero(buff, sizeof(buff));
          getstringfrompayload(buff, &recv_pkt);
          fprintf(stdout, "%s\n", buff);

        }

      } else {
        if (!flag_connection) {

          fprintf(stdout, "<<< Connection to server already closed\n");

        } else {

          fprintf(stderr, "error in reading from the input\n");
          exit(1); //TODO: Think it through

        }
      }
    }

  close(sock);
  return 0;

}

void debug_print_pkt(struct packet * pkt){
  DEBUGS1("===>Printing Pkt<===");
  debug_print_hdr(&(pkt->hdr));
  DEBUGS1("=>Printing Payload<=");
  /*DEBUGS1(pkt->payload); */
}

void debug_print_hdr(struct header *hdr){
  DEBUGS1( "=>Printing Header<=");
  DEBUGN("seq_id", hdr->seq_id);
  DEBUGN("offset", hdr->offset);
  DEBUGN("flag", hdr->flag);
  DEBUGN("checksum", hdr->checksum);
}

void fill_packet( packet* pkt, u_short flag, u_short seq_id, u_short offset, u_char *payload){
	fill_header(pkt, flag, seq_id, offset);
  memset(pkt->payload, 0, sizeof(pkt->payload));
	fill_payload(pkt, payload);
	pkt->hdr.checksum = getchecksum(pkt);
}
void fill_header( packet* pkt, u_short flag, u_short seq_id, u_short offset){
  bzero(pkt, sizeof( packet));
  pkt->hdr.offset = offset;
  pkt->hdr.seq_id = seq_id;
  pkt->hdr.flag = flag;
}

void fill_payload( packet* pkt, u_char *payload){

	bzero(pkt->payload, sizeof(pkt->payload));
  if (pkt->hdr.flag == ACK) {
    // Do Nothing
  } else if (pkt->hdr.flag == READ_RQ || pkt->hdr.flag == WRITE_RQ || pkt->hdr.flag == DL_RQ || pkt->hdr.flag == UNK_RQ) {
    // Sending file name in the packet; only for seq_id == 0
    if (pkt->hdr.seq_id == 0) {
      strcpy(pkt->payload, payload);
      pkt->hdr.offset = (u_short) strlen(payload); // get filename.txt 4:"get "
    }
  } else if (pkt->hdr.flag == WRITE) {
      memcpy(pkt->payload, payload, pkt->hdr.offset);
  }

}


void getstringfrompayload(u_char *buff,  packet * pkt){
  memcpy(buff, pkt->payload, pkt->hdr.offset);
}

ssize_t sendpkt(int sock,  packet *pkt, u_short flag, u_short seq_id, u_short offset, u_char * payload,   sockaddr_in *remote_r, socklen_t remote_length){

  fill_packet(pkt, flag, seq_id, offset, payload);
  return sendwithsock(sock, pkt, remote_r, remote_length);

}

ssize_t waitforpkt(int sock,  packet *prev_pkt,  packet *recv_pkt,  sockaddr_in *remote, socklen_t remote_length, int timeout_flag){
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
    if(nbytes == PACKET_SIZE && checkcksum(recv_pkt)){

      /*// PACKET_SIZE is correct*/
      // TODO: Deliberate More

      if(checkreqflags(recv_pkt) ) break; // Case-1

      if(checkpktflag(prev_pkt, UNK_RQ) && checkpktflag(recv_pkt, WRITE)) break;

      if(checkpktflag(prev_pkt, DL_RQ) && checkpktflag(recv_pkt, ACK)) break;

      if(checkpktflag(prev_pkt, EXIT_RQ) && checkpktflag(recv_pkt, ACK)) break;

      if( checkpktwithwriteresponse(prev_pkt) && checkpktflag(recv_pkt, WRITE))
        if(getpktseqid(recv_pkt) == getpktseqid(prev_pkt) + 1 ) break; // Case-2, 4
        if(checkpktflag(prev_pkt, ACK) && getpktseqid(recv_pkt) == getpktseqid(prev_pkt)){
          DEBUGS1("WARN:ack was lost, resending ack packet");
          debug_print_pkt(prev_pkt);
          nbytes = sendwithsock(sock, prev_pkt, remote, remote_length); // Case-5

        }

      if( checkpkwithackresponse(prev_pkt) && checkpktflag(recv_pkt, ACK))
        if(getpktseqid(prev_pkt) == getpktseqid(recv_pkt)) break;


      if( checkpktflag(prev_pkt, WRITE) && checkpktflag(recv_pkt, ACK) && getpktseqid(recv_pkt) == getpktseqid(prev_pkt) ) break; // case-3

      if( checkpktflag(prev_pkt, NO_FLAG) && checkpktflag(recv_pkt, WRITE) ) { // Case-6
        DEBUGS1("WARN:last ack was lost, resending the ack");
        nbytes = sendpkt(sock, prev_pkt, ACK, getpktseqid(recv_pkt), 0, NULL, remote, remote_length);
        debug_print_pkt(prev_pkt);
        bzero(prev_pkt, sizeof(prev_pkt));

      }

    } else if (nbytes < PACKET_SIZE && nbytes > 0) {

      DEBUGS1("WARN:partial packet received");

      debug_print_pkt(prev_pkt);
      nbytes = sendwithsock(sock, prev_pkt, remote, remote_length);
    } else {

      // for the case when socket timesout
      DEBUGS1("WARN:Socket timeout sending data packet again");
      debug_print_pkt(prev_pkt);
      nbytes = sendwithsock(sock, prev_pkt, remote, remote_length);

    }
  }

  encdecpayload(recv_pkt->payload, recv_pkt->hdr.offset);
  if (timeout_flag) unsetsocktimeout(sock);
  return nbytes;
}


ssize_t sendwithsock(int sock,  packet* pkt,  sockaddr_in* remote, socklen_t remote_length) {
  // TODO: surety that this command is sent as a single chunk

  packet pkt_temp;
  memcpy(&pkt_temp, pkt,sizeof(packet));
  encdecpayload(pkt_temp.payload, pkt_temp.hdr.offset);
  ssize_t nbytes = sendto(sock, &pkt_temp, PACKET_SIZE, 0, (struct sockaddr *)remote, remote_length);
  return nbytes;
}


ssize_t recvwithsock(int sock,  packet* pkt,  sockaddr_in* from_addr, socklen_t* from_addr_length_r) {
  // recvfrom stores the information of sender in from_addr
  // this will keep on blocking in case the messages are lost while in flight

  ssize_t nbytes = recvfrom(sock, pkt, PACKET_SIZE, 0, (struct sockaddr *)from_addr, from_addr_length_r);
  return nbytes;
}


void chunkreadfromsocket(int sock,  packet *sent_pkt,  packet *recv_pkt, char  *file_name,  sockaddr_in *remote, socklen_t remote_length){

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
	u_char payload[PAYLOAD_SIZE];

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

    bzero(payload, sizeof(payload));
    // writing the first data packet received
		memcpy(payload, recv_pkt->payload, recv_pkt->hdr.offset);
    fwrite(payload, sizeof(u_char), recv_pkt->hdr.offset, fp);

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

void chunkwritetosocket(int sock,  packet *sent_pkt,  packet *recv_pkt, char *file_name,  sockaddr_in *remote, socklen_t remote_length) {

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
  u_short offset, seq_id, prev_offset;
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
    prev_offset = offset;
  }

  if(prev_offset == PAYLOAD_SIZE) {
    seq_id = getpktseqid(recv_pkt) + 1;

    offset = 0;
    // send data packet with seq_id =  ack:seq_id + 1
    nbytes = sendpkt(sock, sent_pkt, WRITE, seq_id, offset, NULL, remote, remote_length);

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
  tv.tv_sec = 0;
  tv.tv_usec = 50000; 
  if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval)) != 0)
    perror("unable to set socket timeout");

}

void unsetsocktimeout(int sock){

  struct timeval tv;
  tv.tv_sec = 0; 
  tv.tv_usec = 0; 
  if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval)) != 0)
    perror("unable to unset socket timeout");

}

u_short getpktseqid( packet* pkt){
  return pkt->hdr.seq_id;
}

int checkreqflags( packet* pkt) {
  return checkpktflag(pkt, READ_RQ) || checkpktflag(pkt, WRITE_RQ) || checkpktflag(pkt, LS_RQ) || checkpktflag(pkt, EXIT_RQ) || checkpktflag(pkt, DL_RQ) || checkpktflag(pkt, UNK_RQ);
}

int checkpktflag( packet* pkt, int req) {
  return pkt->hdr.flag == req;
}

int checkpkwithackresponse( packet* pkt){

  return checkpktflag(pkt, WRITE_RQ) || checkpktflag(pkt, EXIT_RQ) || checkpktflag(pkt, DL_RQ) || checkpktflag(pkt, WRITE);
}

int checkpktwithwriteresponse( packet* pkt){

  return checkpktflag(pkt, ACK) || checkpktflag(pkt, READ_RQ) || checkpktflag(pkt, LS_RQ);

}

u_short getchecksum( packet* pkt){

	return cksum((u_short*)pkt, sizeof(pkt)/sizeof(u_short));

}


u_short cksum(u_short *buf, int count){

	register u_long sum = 0;
	while (count--) {
		sum += *buf++;
		if (sum & 0xFFFF0000) {
			/* carry occurred,
			so wrap around */
			sum &= 0xFFFF;
			sum++;
		}
	}
	return ~(sum & 0xFFFF);
}

int checkcksum( packet *pkt) {

	int flag;
	u_short recv_checksum = pkt->hdr.checksum;
	pkt->hdr.checksum = 0;
	flag = (recv_checksum == getchecksum(pkt)) ? TRUE : FALSE;
	pkt->hdr.checksum = recv_checksum;
	return flag;
}

void encdecpayload(u_char *payload, int offset){
  // Simple XOR Key encrypter and decrypter
	int index;
	u_char key = 0x32;

	for(index = 0; index < offset; index++){
		payload[index] = payload[index] ^ key;
	}
}


char* get_second_string(char* string){
  char *temp = (char *) malloc((strlen(string) + 1)*sizeof(char));
  strcpy(temp, string);
  strtok(temp, " ");
  return strtok(NULL, " ");
}

