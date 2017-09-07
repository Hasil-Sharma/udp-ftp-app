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
#define DEBUGN(d, s) fprintf(stderr,"DEBUG: %s: %d\n",d,s)
#define DEBUGS(d, s) fprintf(stderr,"DEBUG: %s: %s\n",d, s)
#define INFON(d, s) fprintf(stdout, "INFO: %s: %d\n", d, s)
#define INFOS(d, s) fprintf(stdout, "INFO: %s: %s\n", d, s)

int main(int argc, char *argv[]){
  int nbytes;
  int sock, read_line;
  char buff[MAXBUFSIZE], command[MAXBUFSIZE];
  unsigned int remote_length, from_addr_length;
  struct sockaddr_in remote, from_addr;
  int flag = TRUE;
  if (argc < 3) {
    fprintf(stderr, "USAGE: <server_ip> <server_port>\n"); 
    exit(1);
  }

  bzero(&remote, sizeof(remote));
  bzero(&buff, sizeof(buff));
  bzero(&command, sizeof(command));

  remote.sin_family = AF_INET;
  remote.sin_port = htons(atoi(argv[2]));
  remote.sin_addr.s_addr = inet_addr(argv[1]); // TODO: Check for the error
  
  remote_length = sizeof(remote); // TODO: Test if changing it before assigment makes a difference
  from_addr_length = sizeof(from_addr);

  if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { fprintf(stderr, "unable to create socket\n"); }
 
  while(flag){

    fprintf(stdout, "Enter the command you want to send:\n");
    if( fgets(command, MAXBUFSIZE, stdin) != NULL ) { 
      
      // This does not breaks up the space delimited sentence 
      // TODO: Surety that this command is sent as a single chunk

      // strlen(command) - 1 to not send the '\n' character
      
      nbytes = sendto(sock, command, strlen(command) - 1, 0, (struct sockaddr *)&remote, remote_length);
      DEBUGN("Sent Bytes to Server", nbytes);

      // recvfrom stores the information of sender in from_addr
      nbytes = recvfrom(sock, buff, MAXBUFSIZE, 0, (struct sockaddr *)&from_addr, &from_addr_length);
      buff[nbytes] = '\0';
      INFOS("Received From Server", buff);
      // Check if from_addr and remote point to same port

      if (strcmp(command, "exit\n") == 0) { flag = FALSE; }
    } else { 
      fprintf(stderr, "error in reading from the input\n"); 
      exit(1); //TODO: Think it through
    }
  }

  close(sock);
  return 0;

}
