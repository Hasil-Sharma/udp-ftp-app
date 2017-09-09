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
  char command[MAXBUFSIZE], *file_name;
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
  bzero(&command, sizeof(command));

  remote.sin_family = AF_INET;
  remote.sin_port = htons(atoi(argv[2]));
  remote.sin_addr.s_addr = inet_addr(argv[1]); // TODO: Check for the error
  
  remote_length = sizeof(remote); // TODO: Test if changing it before assigment makes a difference
  from_addr_length = sizeof(from_addr);

  if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { fprintf(stderr, "unable to create socket\n"); }
  
  struct timeval tv;
  tv.tv_sec = 5;  /* 30 Secs Timeout */
  tv.tv_usec = 0;  // Not init'ing this can cause strange errors
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));

  while(flag){

      bzero(&command, sizeof(command));
      bzero(&sent_pkt, sizeof(sent_pkt));
      bzero(&recv_pkt, sizeof(recv_pkt));

      fprintf(stdout, ">>> ");
      if( fgets(command, MAXBUFSIZE, stdin) != NULL && flag_connection) { 
        
        // This does not breaks up the space delimited sentence 
        
        command[strlen(command) - 1] = '\0'; // remove the trailing \n

        if (strncasecmp(command, "get ", 4) == 0){
          
          seq_id = 0;
          file_name = get_second_string(command);

          // Sending READ Packet
          DEBUGS1("\t\t READ Packet Sent");
          nbytes = sendpkt(sock, &sent_pkt, READ, seq_id, strlen(file_name), file_name, &remote, remote_length);
          debug_print_pkt(&sent_pkt);
          chunkreadfromsocket(sock, &sent_pkt, &recv_pkt, file_name, &remote, remote_length);

        } else if (strncasecmp(command, "put ", 4) == 0){
          
          seq_id = 0;
          file_name = get_second_string(command);

          // Sending WRITE Request

          DEBUGS1("\t\t WRITE Packet Sent");
          nbytes = sendpkt(sock, &sent_pkt, WRITE, seq_id, strlen(file_name), file_name, &remote, remote_length);

          debug_print_pkt(&sent_pkt);
          // Waiting for ACK Packet
          waitforpkt(sock, &sent_pkt, &recv_pkt, &remote, remote_length);
          DEBUGS1("\t\t ACK Packet Received");
          debug_print_pkt(&recv_pkt);
          chunkwritetosocket(sock, &sent_pkt, &recv_pkt, file_name, &remote, remote_length);
        
        } else if (strncasecmp(command, "delete ", 7) == 0){

        
        } else if (strcasecmp(command, "ls") == 0){

        
        } else if (strcasecmp(command, "exit") == 0){
          

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

