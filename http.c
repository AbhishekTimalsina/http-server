#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <magic.h>
#include <errno.h>
#include <signal.h>
 #include <sys/wait.h>


int write_all(int fd, char *data, size_t data_size)
{
    ssize_t written;

    while (data_size > 0)
    {
        written = write(fd, data, data_size);
        if (written < 0) return -1;
        data += written;
        data_size -= written;
    }

    return 0;
}

void sigchild_handler(int signum){
	 siginfo_t infop;
	 
	while(waitid(P_ALL,0,&infop, WEXITED | WNOHANG)==0 && infop.si_pid != 0);
}

char *getFileExtension(char *filename){
	char *ptr = strchr(filename,'.');
    if(ptr==NULL) return "html";
	char *fileExt= ptr;
	fileExt++;

	return fileExt;
}



char *getFileType(char *filename){
	magic_t magic_cookie;
	
	magic_cookie= magic_open(MAGIC_MIME);

    if (magic_cookie == NULL) {
        printf("unable to initialize magic library\n");
        return NULL;
    }

	if (magic_load(magic_cookie, NULL) != 0) {
		printf("cannot load magic database - %s\n", magic_error(magic_cookie));
		magic_close(magic_cookie);
        return NULL;
    }
	
    char *magic_full = magic_file(magic_cookie, filename);
	return magic_full;

}

long renderFile(char *filename,char **buf,char *fileContentType){
    if (fileContentType == NULL || strchr(fileContentType, '/') == NULL) {
        return -1;
    }


	int fileTypeLen= strchr(fileContentType,'/')- fileContentType;
	char fileType[fileTypeLen+1];
	strncpy(fileType,fileContentType,fileTypeLen);

	fileType[fileTypeLen]='\0';


	char *fMode= !strcmp(fileType,"image")?"rb": "r";

	FILE *file= fopen(filename,fMode);
	if(file==NULL){
		return -1;
	}

	fflush(NULL);

	fseek(file,0,SEEK_END);
	long fileSize= ftell(file);
	fseek(file,0,SEEK_SET);
	*buf= malloc(fileSize+1);	
	
	fread(*buf,1,fileSize,file);
;	(*buf)[fileSize]='\0';
	fclose(file);

	return fileSize;
}

void parsePath(char *str, char *field,char *target){
	char method[10];
	sscanf(str,"%s %s",method,target);

}

char* parser(char *str,char *field){
	char *path= malloc(200);
	int firstLineLen= (strstr(str,"\r\n")) - str;
	char firstLine[firstLineLen+1]; 
	strncpy(firstLine,str,firstLineLen);
	firstLine[firstLineLen]= '\0';
	if(!strcmp(field,"method") || !strcmp(field,"path")){
		 parsePath(firstLine, field,path);
		return path;
	}

	
	str+=firstLineLen+2;

	

	while(1){
		if(strncmp(str,"\r\n",2)==0){
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
		str+= (valLen+2);
	}
	return NULL;
}

int handleResponse(){

}

int main(){	
	int BACKLOG=10;
	int enable=1; // enable SO_REUSEADDR
	
	struct addrinfo hints, *res;
	memset(&hints,0,sizeof(hints));
	
	hints.ai_flags= AI_PASSIVE;
	hints.ai_family= AF_INET;
	hints.ai_socktype=SOCK_STREAM;

	int addrInfoCode= getaddrinfo(NULL,"3920", &hints,&res);
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

	printf("Listening in port:3920\r\n");


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
		char clientMsg[5000];


		int bytesRead=  read(clientSocketFd, clientMsg, 4999);
		clientMsg[bytesRead] = '\0';
		char *parsedPath= parser(clientMsg,"path");
		if(parsedPath==NULL) goto parse_error;
		char *filePath= !strcmp(parsedPath,"/")?"index.html": (parsedPath+1);
		char *fileContent;
		
		char fileType[50];
		if(!strcmp(getFileExtension(filePath),"js")){
			strcpy(fileType,"application/javascript");
		}else if(!strcmp(getFileExtension(filePath),"css")){
			strcpy(fileType,"text/css");

		}else{
			strcpy(fileType,getFileType(filePath));
		}
		// printf("%s\n",fileType);
		long fileLength = renderFile(filePath,&fileContent,fileType);
		
		if(fileLength==-1){
			parse_error:
			char response[]="HTTP/1.1 404\r\nContent-Type:text/html; charset=utf-8\r\n\r\n<html><body>404 File not found</body></html>";
			write(clientSocketFd,response,strlen(response));
			close(clientSocketFd);
			exit(EXIT_FAILURE);
		}
		char header[1000];

		sprintf(header,"HTTP/1.1 200\r\nContent-Type:%s;\r\nContent-Length:%ld\r\n\r\n",fileType,fileLength);
		
		write(clientSocketFd,header,strlen(header));
		
		write_all(clientSocketFd,fileContent,fileLength);
		
		close(clientSocketFd);

		exit(EXIT_SUCCESS);
		
		}else{
		
			//  siginfo_t infop;
			//  while(waitid(P_ALL,-1,&infop, WEXITED | WNOHANG)==0 && infop.si_pid != 0);
			close(clientSocketFd);
			// parent
		}
		}

}



// Proper Header
// Learn about C linking and stuff
// fork