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
#include "../headers/packet.h"
#include "../headers/debugutils.h"
#include "../headers/stringutils.h"

#define MAXBUFSIZE 120 // Set the limit on the length of the file
#define TRUE 1
#define FALSE 0

int main(int argc, char *argv[]){
  
  int sock, read_line;
  ssize_t nbytes;

  char buff[MAXBUFSIZE], *file_name;
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

      fprintf(stdout, ">>>");
      if( fgets(buff, MAXBUFSIZE, stdin) != NULL && flag_connection) { 
        
        // This does not breaks up the space delimited sentence 
        
        buff[strlen(buff) - 1] = '\0'; // remove the trailing \n

        if (strncasecmp(buff, "get ", 4) == 0){
          
          seq_id = 0;
          
          file_name = get_second_string(buff);
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
          file_name = get_second_string(buff);

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

        
        } else if (strcasecmp(buff, "ls") == 0){
          
          seq_id = 0;

          nbytes = sendpkt(sock, &sent_pkt, LS_RQ, seq_id, 0, NULL, &remote, remote_length);

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
        } else {


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

