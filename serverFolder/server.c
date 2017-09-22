#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <glob.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include "../headers/packet.h"
#include "../headers/debugutils.h"

#define MAXBUFSIZE 100
#define TRUE 1
#define FALSE 0

char *getdir();

int main(int argc, char *argv[]){

  int sock, flag = TRUE; 
  ssize_t nbytes;
  u_short seq_id; 
  struct sockaddr_in sin, remote;
  socklen_t remote_length, offset;
  u_char  *buff;
  char file_name[MAXBUFSIZE];
  struct packet sent_pkt, recv_pkt;
  FILE *fp;

  if (argc != 2){
    fprintf(stderr, "USAGE : <port>\n"); // TODO: make it print via stderr
    exit(1);
  }

  bzero(&sin, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(atoi(argv[1])); // TODO: make sure the user enter a number
  sin.sin_addr.s_addr = INADDR_ANY;

  if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    fprintf(stderr, "unable to bind to socket\n"); 
    exit(1);
  }

  if(bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
    fprintf(stderr, "unable to bind to socket\n"); 
    exit(1);
  }

  
  while(flag){

    bzero(&remote, sizeof(remote));
    bzero(file_name, sizeof(file_name));
    remote_length = sizeof(remote);
    bzero(&recv_pkt, sizeof(recv_pkt)); 
    bzero(&sent_pkt, sizeof(sent_pkt));

    nbytes = waitforpkt(sock, &sent_pkt, &recv_pkt, &remote, remote_length, FALSE);

    DEBUGS1("Request Packet Received"); 
    debug_print_pkt(&recv_pkt);
    if (checkpktflag(&recv_pkt, READ_RQ)) {

      DEBUGS1("\t\tRequest is GET");
      // Get name of the file from buffer
      getstringfrompayload(file_name, &recv_pkt);
      chunkwritetosocket(sock, &sent_pkt, &recv_pkt, file_name, &remote, remote_length);
        
    } else if(checkpktflag(&recv_pkt, WRITE_RQ)) {
     
        seq_id = getpktseqid(&recv_pkt);

        DEBUGS1("\t\tRequest is PUT");
        getstringfrompayload(file_name, &recv_pkt);

        // Send WRITE ACK for WRITE Packet from Client
        nbytes = sendpkt(sock, &sent_pkt, ACK, seq_id, 0, NULL, &remote, remote_length);

        DEBUGS1("\t\tACK Packet Sent");
        debug_print_pkt(&sent_pkt);
       
        // Waiting for WRITE ACK Reponse : Data Packet 
        // Resend WRITE ACK Reponse if no Data Packet
        nbytes = waitforpkt(sock, &sent_pkt, &recv_pkt, &remote, remote_length, TRUE);
        chunkreadfromsocket(sock, &sent_pkt, &recv_pkt, file_name, &remote, remote_length);

    } else if(checkpktflag(&recv_pkt, LS_RQ)) {

      DEBUGS1("\t\tRequest is LS"); 
      seq_id = getpktseqid(&recv_pkt);
      buff = (u_char *)getdir(); 

      // Send DATA Packet
      nbytes = sendpkt(sock, &sent_pkt, WRITE, seq_id+1, strlen(buff), buff, &remote, remote_length);
      free(buff);  

    } else if (checkpktflag(&recv_pkt, EXIT_RQ)) {

      seq_id = getpktseqid(&recv_pkt);
      DEBUGS1("\t\tRequest is EXIT");
      nbytes = sendpkt(sock, &sent_pkt, ACK, seq_id, 0, NULL, &remote, remote_length);
      flag = FALSE;

    } else if (checkpktflag(&recv_pkt, DL_RQ)) {

      seq_id = getpktseqid(&recv_pkt);

      DEBUGS1("\t\tRequest is DELETE");

      getstringfrompayload(file_name, &recv_pkt);
      debug_print_pkt(&recv_pkt);
      DEBUGS1(file_name);
      if(remove(file_name) != 0){

        INFOS("Deleted", file_name);
      } 
    }else {
      flag = FALSE;
    } 
  }

  close(sock);
  return 0;
}

char *getdir(){
  u_int i;
  char * result;
  result = (char *) malloc(MAXBUFSIZE*sizeof(char));

  bzero(result, MAXBUFSIZE);

  glob_t glob_result;
  glob("./*", GLOB_TILDE, NULL, &glob_result);

  for(i = 0; i < glob_result.gl_pathc; ++i){
    strcat(result, glob_result.gl_pathv[i]);
    strcat(result, "\t");
  }

  return result;
}
