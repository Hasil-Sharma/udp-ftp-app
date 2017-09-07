#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
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

#define MAXBUFSIZE 100
#define TRUE 1
#define FALSE 0
#define DEBUGN(d, s) fprintf(stderr,"DEBUG: %s: %d\n", d, s)
#define DEBUGS(d, s) fprintf(stderr,"DEBUG: %s: %s\n", d, s)
#define INFON(d, s) fprintf(stdout, "INFO: %s: %d\n", d, s)
#define INFOS(d, s) fprintf(stdout, "INFO: %s: %s\n", d, s)

char* getdir();

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
     
     if (strcasecmp(buff, "3") == 0) {
       // populate the files in directory
       strcpy(command , getdir());
     } else if (strncasecmp(buff, "get ", 4) == 0) {
      // method for the get
       strcpy(command , "return of get");
     } else if (strncasecmp(buff, "put ", 4) == 0) {
      // method for the put
       strcpy(command , "return of put");
     } else if (strncasecmp(buff, "delete ", 7) == 0) {
      // method for delete
       strcpy(command , "return of delete");
     } else if (strcasecmp(buff, "4") == 0) {
      // Server should shut down gracefully
       flag = FALSE;
       continue;
     } else {
       sprintf(command, "Command not understood: %s", buff);
      // the server should simply repeat the command back to the
      // client with no modification, stating that the given command was not understood.
     }

     nbytes = sendto(sock, command, strlen(command), 0, (struct sockaddr *)&remote, remote_length);
     DEBUGS("Returning String", command);
     DEBUGN("Sent Bytes to Client", nbytes);
 }

 close(sock);
 return 0;
}

char *getdir(){
	DIR *d;
	char * result;
	result = (char *) malloc(MAXBUFSIZE*sizeof(char));
	bzero(result, sizeof(char *));
  int flag = 0;
	struct dirent *dir;
	d = opendir("./files");
	if (d){
		while ((dir = readdir(d)) != NULL){
			if(strcmp(dir->d_name, ".") && strcmp(dir->d_name, "..")){
				if(flag) strcat(result, "\t");		
				strcat(result, dir->d_name);
				flag = 1;
			}
		}
		closedir(d); 
	}

	return result;
}









