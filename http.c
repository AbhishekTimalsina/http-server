#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <magic.h>

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

char *getFileExtension(char *filename){
	char *fileExt= strchr(filename,'.')+1;
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

	listen(sockFd,BACKLOG);
	printf("Listening in port:3920\r\n");


	while(1){

		int clientSocketFd=accept(sockFd,NULL,NULL);
		 int bytesRead=  read(clientSocketFd, clientMsg, 4999);
		// fflush(NULL);
		clientMsg[bytesRead] = '\0';
		char *parsedPath= parser(clientMsg,"path");
		if(parsedPath==NULL) goto Error;
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
		long fileLength = renderFile(filePath,&fileContent,fileType);
	
		Error:
		if(fileLength==-1){
			char response[]="HTTP/1.1 404\r\nContent-Type:text/html; charset=utf-8\r\n\r\n<html><body>404 File not found</body></html>";
			write(clientSocketFd,response,strlen(response));
			close(clientSocketFd);
			continue;
		}
		char header[1000];


		sprintf(header,"HTTP/1.1 200\r\nContent-Type:%s;\r\nContent-Length:%ld\r\n\r\n",fileType,fileLength);

		write(clientSocketFd,header,strlen(header));

		write_all(clientSocketFd,fileContent,fileLength);

		close(clientSocketFd);
	}

}





