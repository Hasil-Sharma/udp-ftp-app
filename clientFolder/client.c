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

#define MAXBUFSIZE 100
#define TRUE 1
#define FALSE 0
int main(int argc, char *argv[]){
  int nbytes;
  int sock, read_line;
  char buffer[MAXBUFSIZE], command[MAXBUFSIZE];
  unsigned int remote_length, from_addr_length;
  struct sockaddr_in remote, from_addr;
  int flag = TRUE;
  if (argc < 3) {
    fprintf(stderr, "USAGE: <server_ip> <server_port>\n"); 
    exit(1);
  }

  bzero(&remote, sizeof(remote));
  remote.sin_family = AF_INET;
  remote.sin_port = htons(atoi(argv[2]));
  remote.sin_addr.s_addr = inet_addr(argv[1]); // TODO: Check for the error
  
  remote_length = sizeof(remote); // TODO: Test if changing it before assigment makes a difference

  if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { fprintf(stderr, "unable to create socket\n"); }
 
  while(flag){

    fprintf(stdout, "Enter the command you want to send:\n");
    if( fgets(command, MAXBUFSIZE, stdin) != NULL ) { 
      // This does not breaks up the space delimited sentence 
      // TODO: Surety that this command is sent as a single chunk

      // strlen(command) - 1 to not send the '\n' character
      nbytes = sendto(sock, command, strlen(command) - 1, 0, (struct sockaddr *)&remote, remote_length);
      fprintf(stderr, "Sent %d bytes\n", nbytes); // AUX

      if (strcmp(command, "exit\n") == 0) { flag = FALSE; }
    } else { 
      fprintf(stderr, "error in reading from the input\n"); 
      exit(1); //TODO: Think it through
    }

    //from_addr_length = sizeof(from_addr);
    //nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *)&from_addr, &from_addr_length);
    //printf("Server says %s\n", buffer);

  }

  close(sock);

}
