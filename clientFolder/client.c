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
  
  int nbytes, sock, read_line;
  char command[MAXBUFSIZE], *file_name;
  unsigned int remote_length, from_addr_length;
  uint16_t seq_id;
  struct sockaddr_in remote, from_addr;
  struct packet send_pkt, recv_pkt;
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
      fprintf(stdout, ">>> ");
      if( fgets(command, MAXBUFSIZE, stdin) != NULL && flag_connection) { 
        
        // This does not breaks up the space delimited sentence 
        
        command[strlen(command) - 1] = '\0'; // remove the trailing \n

        if (strncasecmp(command, "get ", 4) == 0){
          
          seq_id = 0;
          file_name = get_second_string(command);
          nbytes = sendpkt(sock, &send_pkt, READ, seq_id++, 0, file_name, &remote, remote_length);
          DEBUGN("Sent Bytes to Server", nbytes);
          nbytes = waitforpkt(sock, &send_pkt, &recv_pkt, &from_addr, &from_addr_length, &remote, remote_length);
          fp = fopen(file_name, "wb");
          while(TRUE){
            fwrite(recv_pkt.payload, sizeof(schar), recv_pkt.hdr.offset, fp);
            if (recv_pkt.hdr.offset < PAYLOAD_SIZE) break;
            // send ACK for Data Packt : seq_id
            nbytes = sendpkt(sock, &send_pkt, ACK, seq_id++, 0,  NULL, &remote, remote_length);
            nbytes = waitforpkt(sock, &send_pkt, &recv_pkt, &from_addr, &from_addr_length, &remote, remote_length);
          }
          fclose(fp);
          DEBUGS1("File Received");

        } else if (strncasecmp(command, "put ", 4) == 0){
          
          /*seq_id = 0;*/
          /*fill_header(&send_pkt, 2, seq_id);  */
          
          /*nbytes = sendwithsock(sock, "1", &remote, remote_length);*/
          /*DEBUGN("Sent Bytes to Server", nbytes);*/
          /*nbytes = recvwithsock(sock, buff, &from_addr, &from_addr_length);*/
          /*buff[nbytes] = '\0';*/
          /*fprintf(stdout, "<<< %s\n", buff);*/
        
        } else if (strncasecmp(command, "delete ", 7) == 0){

          /*seq_id = 0;*/
          /*fill_header(&send_pkt, 3, seq_id);  */
          
          /*nbytes = sendwithsock(sock, "2", &remote, remote_length);*/
          /*DEBUGN("Sent Bytes to Server", nbytes);*/
          /*nbytes = recvwithsock(sock, buff, &from_addr, &from_addr_length);*/
          /*buff[nbytes] = '\0';*/
          /*fprintf(stdout, "<<< %s\n", buff);*/
        
        } else if (strcasecmp(command, "ls") == 0){

          /*seq_id = 0;*/
          /*fill_header(&send_pkt, 4, seq_id);  */
          
          /*nbytes = sendwithsock(sock, "3", &remote, remote_length);*/
          /*DEBUGN("Sent Bytes to Server", nbytes);*/
          /*nbytes = recvwithsock(sock, buff, &from_addr, &from_addr_length);*/
          /*buff[nbytes] = '\0';*/
          /*fprintf(stdout, "<<< %s\n", buff);*/
        
        } else if (strcasecmp(command, "exit") == 0){
          
          /*seq_id = 0;*/
          /*fill_header(&send_pkt, 5, seq_id);  */
          
          /*nbytes = sendwithsock(sock, "4", &remote, remote_length);*/
          /*DEBUGN("Sent Bytes to Server", nbytes);*/
          /*flag_connection = FALSE;*/

        } else {

          /*seq_id = 0;*/
          /*fill_header(&send_pkt, 6, seq_id);  */
          
          /*nbytes = sendwithsock(sock, command, &remote, remote_length);*/
          /*DEBUGN("Sent Bytes to Server", nbytes);*/
          /*nbytes = recvwithsock(sock, buff, &from_addr, &from_addr_length);*/
          /*buff[nbytes] = '\0';*/
          /*fprintf(stdout, "<<< %s\n", buff);*/

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

