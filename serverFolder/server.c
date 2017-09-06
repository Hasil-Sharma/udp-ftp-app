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

 int sock;
 int flag = TRUE;
 struct sockaddr_in sin, remote;
 unsigned int remote_length;
 int nbytes;
 char buff[MAXBUFSIZE];
 
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
 remote_length = sizeof(remote);

 //wait for an incoming message
 bzero(buff, sizeof(buff));

 while(flag){
   nbytes = recvfrom(sock, buff, MAXBUFSIZE, 0, (struct sockaddr *)&remote, &remote_length);
   buff[nbytes] = '\0';
   fprintf(stdout, "Received: %s\n", buff);
   
   // TODO :make it case-insensitive

   if (strcmp(buff, "exit") == 0) {
     flag = FALSE;
   } else if (strcmp(buff, "ls") == 0) {

   }
 }

 close(sock);
 return 0;
}

