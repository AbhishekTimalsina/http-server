#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include "Handle_Response.h"

char PORT[6]="8080";

void sigchild_handler(){
	 siginfo_t infop;
	 
	while(waitid(P_ALL,0,&infop, WEXITED | WNOHANG)==0 && infop.si_pid != 0);
}


int main(int argc, char *argv[]){	
	if(argc>1){
		strcpy(PORT,argv[1]);
	}
	int BACKLOG=10;
	int enable=1; // enable SO_REUSEADDR

	
	
	struct addrinfo hints, *res;
	memset(&hints,0,sizeof(hints));
	
	hints.ai_flags= AI_PASSIVE;
	hints.ai_family= AF_INET;
	hints.ai_socktype=SOCK_STREAM;

	int addrInfoCode= getaddrinfo(NULL,PORT, &hints,&res);
	if(addrInfoCode!=0){
		printf("%s",gai_strerror(addrInfoCode));
		return 1;
	}

	int sockFd=socket(AF_INET,SOCK_STREAM,0);

	if(sockFd==-1){
		perror("Error in socket");
		close(sockFd);

		return 1;
	}

	if(setsockopt(sockFd,SOL_SOCKET,SO_REUSEADDR,&enable,sizeof(enable))==-1){
		perror("Error in setsockopt");
		return 1;
	}


		if(bind(sockFd,res->ai_addr,res->ai_addrlen)==-1){
			if(errno== EADDRINUSE){
				printf("Port is already in Use");
			}else{
				perror("Error in bind");
			}
			close(sockFd);
			return 1;
		}

	if(listen(sockFd,BACKLOG)==-1){
		perror("Error in socket");
		close(sockFd);
		return 1;
	}
	
	struct sigaction act;
	act.sa_handler= sigchild_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_RESTART; 
	sigaction(SIGCHLD,&act, NULL);

	printf("Listening in port:%s\r\n",PORT);


	while(1){

		int clientSocketFd=accept(sockFd,NULL,NULL);
			if(clientSocketFd==-1){
				perror("Error in accept");
			close(sockFd);
			return 1;
			}
		int pid;
		if((pid=fork())<0){
			printf("Error creating fork");
		}else if(pid==0){
		close(sockFd);
		handleResponse(clientSocketFd);
		
		}else{
			close(clientSocketFd);
		}
		}

}



// Proper Header