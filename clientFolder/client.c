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

int sendwithsock(int, char*, struct sockaddr_in*, unsigned int); 
int recvwithsock(int, char*, struct sockaddr_in*, unsigned int*);

int main(int argc, char *argv[]){
  int nbytes;
  int sock, read_line;
  char buff[MAXBUFSIZE], command[MAXBUFSIZE];
  unsigned int remote_length, from_addr_length;
  struct sockaddr_in remote, from_addr;
  int flag = TRUE, flag_connection = TRUE;

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
      fprintf(stdout, ">>> ");
      if( fgets(command, MAXBUFSIZE, stdin) != NULL && flag_connection) { 
        
        // This does not breaks up the space delimited sentence 
        // TODO: Surety that this command is sent as a single chunk
        
        command[strlen(command) - 1] = '\0'; // remove the trailing \n

        if (strncasecmp(command, "get ", 4) == 0){

          nbytes = sendwithsock(sock, "0", &remote, remote_length);
          DEBUGN("Sent Bytes to Server", nbytes);
          nbytes = recvwithsock(sock, buff, &from_addr, &from_addr_length);
          buff[nbytes] = '\0';
          fprintf(stdout, "<<< %s\n", buff);

        } else if (strncasecmp(command, "put ", 4) == 0){

          nbytes = sendwithsock(sock, "1", &remote, remote_length);
          DEBUGN("Sent Bytes to Server", nbytes);
          nbytes = recvwithsock(sock, buff, &from_addr, &from_addr_length);
          buff[nbytes] = '\0';
          fprintf(stdout, "<<< %s\n", buff);
        
        } else if (strncasecmp(command, "delete ", 7) == 0){

          nbytes = sendwithsock(sock, "2", &remote, remote_length);
          DEBUGN("Sent Bytes to Server", nbytes);
          nbytes = recvwithsock(sock, buff, &from_addr, &from_addr_length);
          buff[nbytes] = '\0';
          fprintf(stdout, "<<< %s\n", buff);
        
        } else if (strcasecmp(command, "ls") == 0){

          nbytes = sendwithsock(sock, "3", &remote, remote_length);
          DEBUGN("Sent Bytes to Server", nbytes);
          nbytes = recvwithsock(sock, buff, &from_addr, &from_addr_length);
          buff[nbytes] = '\0';
          fprintf(stdout, "<<< %s\n", buff);
        
        } else if (strcasecmp(command, "exit") == 0){

          nbytes = sendwithsock(sock, "4", &remote, remote_length);
          DEBUGN("Sent Bytes to Server", nbytes);
          flag_connection = FALSE;

        } else {

          nbytes = sendwithsock(sock, command, &remote, remote_length);
          DEBUGN("Sent Bytes to Server", nbytes);
          nbytes = recvwithsock(sock, buff, &from_addr, &from_addr_length);
          buff[nbytes] = '\0';
          fprintf(stdout, "<<< %s\n", buff);

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

int sendwithsock(int sock, char *command, struct sockaddr_in *remote, unsigned int remote_length) {
  // TODO: Surety that this command is sent as a single chunk
  return sendto(sock, command, strlen(command), 0, (struct sockaddr *)remote, remote_length);
}

int recvwithsock(int sock, char *buff, struct sockaddr_in * from_addr, unsigned int * from_addr_length) {
  // recvfrom stores the information of sender in from_addr
  // This will keep on blocking in case the messages are lost while in flight
  return recvfrom(sock, buff, MAXBUFSIZE, 0, (struct sockaddr *)from_addr, from_addr_length);
}
