#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <magic.h>
#include <errno.h>

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
	
    char *magic_full = (char *)magic_file(magic_cookie, filename);
	char *path= strdup(magic_full);
	magic_close(magic_cookie);
	return path;

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

void parsePath(char *str, char method[],char *path){
	sscanf(str,"%s %s",method,path);
}

char* parser(char *str,char *field){
	int firstLineLen= (strstr(str,"\r\n")) - str;
	char firstLine[firstLineLen+1]; 
	char *path= malloc(firstLineLen);
    char method[firstLineLen];
	strncpy(firstLine,str,firstLineLen);
	firstLine[firstLineLen]= '\0';
	if(!strcmp(field,"method") || !strcmp(field,"path")){
		 parsePath(firstLine, method,path);
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

int handleResponse(int clientSocketFd){
		char clientMsg[5000];


		int bytesRead=  read(clientSocketFd, clientMsg, 4999);
		clientMsg[bytesRead] = '\0';
		char *parsedPath= parser(clientMsg,"path");

		if(parsedPath==NULL) goto not_found;

		char *inPath= !strcmp(parsedPath,"/")?"/index.html": (parsedPath);
		char *filePath= malloc(4+strlen(inPath));
		strcpy(filePath,"site");
		strcat(filePath,inPath);


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
		free(parsedPath);
		free(filePath);

		
		if(fileLength==-1){
			not_found: ;
			char response[]="HTTP/1.1 404\r\nContent-Type:text/html; charset=utf-8\r\n\r\n<html><body>404 File not found</body></html>";
			write(clientSocketFd,response,strlen(response));
			close(clientSocketFd);
			exit(EXIT_FAILURE);
		}
		char header[1000];

		sprintf(header,"HTTP/1.1 200\r\nContent-Type:%s;\r\nContent-Length:%ld\r\n\r\n",fileType,fileLength);
		
		write(clientSocketFd,header,strlen(header));
		
		write_all(clientSocketFd,fileContent,fileLength);
		
		free(fileContent);
		close(clientSocketFd);

		exit(EXIT_SUCCESS);
}


