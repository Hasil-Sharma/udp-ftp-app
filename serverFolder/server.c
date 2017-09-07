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
#define DEBUGN(d, s) fprintf(stderr,"DEBUG: %s: %d\n", d, s)
#define DEBUGS(d, s) fprintf(stderr,"DEBUG: %s: %s\n", d, s)
#define INFON(d, s) fprintf(stdout, "INFO: %s: %d\n", d, s)
#define INFOS(d, s) fprintf(stdout, "INFO: %s: %s\n", d, s)

int main(int argc, char *argv[]){

 int sock;
 int flag = TRUE;
 struct sockaddr_in sin, remote;
 unsigned int remote_length;
 int nbytes;
 char buff[MAXBUFSIZE], command[MAXBUFSIZE];
 
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

 bzero(&remote, sizeof(remote));
 bzero(buff, sizeof(buff));
 bzero(command, sizeof(command));

 remote_length = sizeof(remote);
 while(flag){

      // This will keep on blocking in case the messages are lost while in flight
     nbytes = recvfrom(sock, buff, MAXBUFSIZE, 0, (struct sockaddr *)&remote, &remote_length);
     buff[nbytes] = '\0';
     
     DEBUGS("Received from Client", buff);
     fprintf(stdout, "Enter the command you want to send:\n");
     if( fgets(command, MAXBUFSIZE, stdin) != NULL ) {

        // This does not breaks up the space delimited sentence 
        // TODO: Surety that this command is sent as a single chunk

        // strlen(command) - 1 to not send the '\n' character

       nbytes = sendto(sock, command, strlen(command) - 1, 0, (struct sockaddr *)&remote, remote_length);
       DEBUGN("Sent Bytes to Client", nbytes);
       bzero(&remote, sizeof(remote));
     // TODO :make it case-insensitive

   } else {
     fprintf(stderr, "error in reading from the input\n");
     exit(1);
   }
 }
 close(sock);
 return 0;
}

