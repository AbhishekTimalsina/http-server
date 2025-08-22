#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>


int renderFile(char **buf){
	printf("This is running");

	FILE *file= fopen("index.html","r");

	fflush(NULL);

	fseek(file,0,SEEK_END);
	long fileSize= ftell(file);
	fseek(file,0,SEEK_SET);
	printf("%ld",fileSize);
	*buf= malloc(sizeof(char)* fileSize);	
	char ch;

	fread(*buf,1,fileSize,file);
	// int i=0;
	// while((ch=fgetc(file))!=EOF){
	// 	(*buf)[i]= ch;
	// 	i++;
	// }
	// strcpy(buf,filBbuf);
	fclose(file);
	// printf("%s",buffer);

	return fileSize;
	
	// return buffer;

}

void parsePath(char *str, char *field,char *target){
	char method[10];
	sscanf(str,"%s %s",method,target);

}

void parser(char *str,char *field){
	char path[200];
	int firstLineLen= (strstr(str,"\r\n")) - str;
	char firstLine[firstLineLen+1]; 
	strncpy(firstLine,str,firstLineLen);
	firstLine[firstLineLen]= '\0';
	if(!strcmp(field,"method") || !strcmp(field,"path")){
		 parsePath(firstLine, field,path);
	
	}

	
	str+=firstLineLen+2;

	

	while(1){
		if(strncmp(str,"\r\n",2)==0){
			// printf("End of Input");
			break;
		}
		char *newstr=strstr(str,": ");
		int keyLen=(newstr-str); 	
		char key[(keyLen+1)];
		strncpy(key,str,keyLen);
		key[keyLen]='\0';
		str+= keyLen+2;
		
		
		int valLen= strstr(str,"\r\n")- str;
		char valStr[valLen+1];
		strncpy(valStr,str,valLen);
		valStr[valLen]='\0';
		// printf("\tValue: %s\n",valStr);
		str+= (valLen+2);
	}

}

int main(){	
	int BACKLOG=10;
	int enable=1; // enable SO_REUSEADDR
	char clientMsg[5000];
	
	struct addrinfo hints, *res;
	memset(&hints,0,sizeof(hints));
	
	hints.ai_flags= AI_PASSIVE;
	hints.ai_family= AF_INET;
	hints.ai_socktype=SOCK_STREAM;

	
	getaddrinfo(NULL,"3920", &hints,&res);

	int sockFd=socket(AF_INET,SOCK_STREAM,0);

	setsockopt(sockFd,SOL_SOCKET,SO_REUSEADDR,&enable,sizeof(enable));

	bind(sockFd,res->ai_addr,res->ai_addrlen);

	// while(1){
	listen(sockFd,BACKLOG);
	printf("Listening in port:3920\r\n");


	while(1){

		int clientSocketFd=accept(sockFd,NULL,NULL);
		// printf("client socket id is: %d\n",clientSocketFd);
		// fflush(NULL);
		 int bytesRead=  read(clientSocketFd, clientMsg, 4999);
		// fflush(NULL);
		clientMsg[bytesRead] = '\0';
		// printf("'%s'\n", clientMsg); 
		
		// parser(clientMsg,"path");
		char *fileContent;
		int fileLength= renderFile(&fileContent);
		fflush(NULL);
		char response[1000]="HTTP/1.1 200\r\nContent-Type:text/html; charset=utf-8\r\n\r\n";
		printf("%s",fileContent);
		strcat(response,fileContent);
		
		write(clientSocketFd,response,strlen(response));

		close(clientSocketFd);
	}

	printf("Hello world\n");
}





