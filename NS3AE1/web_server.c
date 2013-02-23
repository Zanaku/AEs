#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <locale.h>
#include <langinfo.h>
#include <pthread.h>
#include <limits.h>
#include <errno.h>

/*===CONNECTION CREATION===*/

int initialiseFd(){
    int fd = socket(AF_INET, SOCK_STREAM,0);
    if (fd == -1) {
        printf("Error: Failed to create socket.");
    }
    printf("Initialised FD...\n");
    return fd;
}   

struct sockaddr_in defineSocket(struct sockaddr_in addr, int port){
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    printf("Defined Socket...\n");
    return addr;
}

void bindSocket(int fd, struct sockaddr_in addr){
    if (bind(fd, (struct sockaddr *) &addr, sizeof(addr))==-1) {
        printf("Error: Binding of socket failed.\nRetrying...\n");
        //fprintf(stderr,"Errno %d: %s\n",errno,strerror(errno));
        sleep(5);
        bindSocket(fd,addr);
        return;
    }
    printf("Bound Socket...\n");
}

void listenFd(int fd, int i){
    if (listen(fd,i)==-1){
        printf("Error: Could not listen for connections.");
    }
    printf("Began Listening For Connections...\n");
}

int initialiseConnFd(int fd, struct sockaddr_in cliaddr, socklen_t cliaddrlen){
    int connfd = accept(fd, (struct sockaddr *) &cliaddr, &cliaddrlen);
    if(connfd == -1){
        printf("Error: Connection acceptance failed.");
    }
    printf("Initialised Connection FD...\n");
    return connfd;
}

/*===REQUEST MANAGEMENT===*/

void writeBadRequest(int connfd,char* protocol){
    struct stat fs;
    char response[1024];
    char* page; 
    int fd = open("400.html", O_RDONLY);
    if(fstat(fd,&fs)==-1){printf("Could Not retrieve file size in Bad Request.\n");}
    page = (char*)malloc(fs.st_size+1);
    read(fd,page,fs.st_size);
    int plen = strlen(page);
    page[plen]='\0';
    printf("400 File Size:%d\n",(int)fs.st_size);
    sprintf(response,"%s 400 Bad Response\nContent-Type: text/html\nConnection: close\nContent-Length: %d\r\n\r\n%s",protocol,(int)fs.st_size,page);
    plen = strlen(response);
    response[plen]='\0';
    if(write(connfd,response,strlen(response))==-1){fprintf(stderr,"400 Errno %d: %s\n",errno,strerror(errno));}
    printf("Wrote:\n%s\n",response);
    close(fd);
    free(page);
}

void writeFileNotFound(int connfd,char* protocol){
    struct stat fs;
    char response[1024];
    char* page; 
    int fd = open("404.html", O_RDONLY);
    if(fstat(fd,&fs)==-1){printf("Could Not retrieve file size in 404.\n");}
    page = (char*)malloc(fs.st_size+1);
    ssize_t plen = read(fd,page,fs.st_size);
    page[plen]='\0';
    printf("404 File Size:%d\n",(int)fs.st_size);
    sprintf(response,"%s 404 File Not Found\nContent-Type: text/html\nConnection: close\nContent-Length: %d\r\n\r\n%s",protocol,(int)fs.st_size,page);
    plen = strlen(response);
    response[plen]='\0';
    if(write(connfd,response,strlen(response))==-1){fprintf(stderr,"404 Errno %d: %s\n",errno,strerror(errno));}
    printf("Wrote:\n%s\n",response);
    close(fd);
    free(page);
}

void writeInternal(int connfd,char* protocol){
    char response[1024];
    sprintf(response,"%s 500 Internal Server Error\nContent-Type: text/html\nConnection: close\nContent-Length: 237\r\n\r\n<HTML><!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\"\"http://www.w3.org/TR/html4/strict.dtd\"><HTML><HEAD><TITLE> 500 Internal Server Error </TITLE></HEAD><BODY> The server has encountered an internal error. Sorry! :( </BODY><HTML>",protocol);
    ssize_t plen = strlen(response);
    response[plen]='\0';
    if(write(connfd,response,strlen(response))==-1){fprintf(stderr,"500 Errno %d: %s\n",errno,strerror(errno));}
    printf("Wrote:\n%s\n",response);
}

char* contentType(char* url){
    /*Generates content type for return based on the extension of the url requested. Only supports file types listed in handout.*/
    char* ctype;
    char* extension;
    extension = strstr(url,".");
    if(strcmp(extension,".htm")==0 || strcmp(extension,".html")==0){
        ctype = "text/html";   
    }
    else if(strcmp(extension,".jpg")==0 || strcmp(extension,".jpeg")==0){
        ctype = "image/jpeg";   
    }
    else if(strcmp(extension,".gif")==0){
        ctype = "image/gif";   
    }
    else if(strcmp(extension,".txt")==0){
        ctype = "text/plain";   
    }
    else{ctype = "application/octet-stream";}
    return ctype;

}

void writeRequested(int connfd,char* protocol,char* url){
    struct stat fs;
    char response[1024];
    char* page; 
    char* contType;
    contType = contentType(url);

    int fd = open(url+1, O_RDONLY);
    if(fd==-1){
        writeFileNotFound(connfd,protocol);
        return;
    }
    if(fstat(fd,&fs)==-1){
        printf("Could Not retrieve file size!\n");
        writeInternal(connfd,protocol);
    }

    page = (char*)malloc(fs.st_size);
    read(fd,page,fs.st_size);

    sprintf(response,"%s 200 OK\nContent-Type: %s\nContent-Length: %d\r\n\r\n",protocol,contType,(int)fs.st_size);

    if(write(connfd,response,strlen(response))==-1){
        fprintf(stderr,"200 Errno %d: %s\n",errno,strerror(errno));
    }
    if(write(connfd,page,fs.st_size)==-1){
        fprintf(stderr,"200 Errno %d: %s\n",errno,strerror(errno));
    }
    printf("Returning Response from Thread %lu\n",pthread_self());
    printf("****Writing Response****\n%s\n",response);
    close(fd);
    free(page); 
}

int verifyGet(char* get){
    /*Check that GET starts*/
    if(strncmp(get,"GET",3)!=0){
        printf("Bad GET\n");
        return 0;   
    }
    else{return 1;}
}

void* readRequest(void* voidconn){
    /*Sets connfd to the passed connfd that is malloc'd outside*/
    int connfd = *(int*)voidconn;    
    /*Mallocs a new buffer to hold the read data.*/    
    char* buf;
    ssize_t bufsize = 1024;
    buf=(char*)malloc(bufsize+1);        
    /*rcount holds the value returned by read, while totalRead keeps track of the total amount read for this request.*/    
    ssize_t rcount;
    ssize_t totalRead=0;

    printf("Reading from Client...\n");
    while((rcount = read(connfd, buf+totalRead,(bufsize)-totalRead))!=0){
        if (rcount == -1) {
            printf("Error: failed to read from connection!\n");
            fprintf(stderr,"Errno %d: %s\n",errno,strerror(errno));
            break;
        }
        /*Update the total read counter, and if necessary increase the buffer size. Afterwards add a null character to the end of the buffer.*/
        totalRead+=rcount;
        if(totalRead >= bufsize){
            buf = (char*)realloc(buf,bufsize*2+1);
            bufsize=bufsize*2+1;
        }
        buf[totalRead] = '\0';
        
        /*Looks for the end of the request, and if found processes.*/
        if(strstr(buf,"\r\n\r\n")!=NULL){
            printf("****Received Request****\n%s",buf);
            /*Sets up buffers to contain GET, URL, and Protocol. Sizes were made large enough to hopefully contain most sensible requests. 
             *Hosttmp is created with a size to hold the maximum possible host name, as well as a colon followed by a port.*/ 
            char get[1024];
            char url[1024];
            char protocol[1024];
            char hosttmp[HOST_NAME_MAX+6];            

            /*Strips expected GET, URL, and Protocol from first line*/
            int splitval;
            splitval = sscanf(buf,"%s %s %s\r\n",get,url,protocol);
     
            /*Creates pointers to the location of the Host line and the start of the port segment of this*/
            char* hostLoc = strstr(buf,"Host:");
            char* portStart;

            /*Strips out port number from host name, puts portless hostname in host buffer*/
            char host[HOST_NAME_MAX];
            splitval = sscanf(hostLoc,"Host: %s\r\n",hosttmp);
            portStart = strstr(hosttmp,":");

            size_t portLoc = portStart - hosttmp;

            strncpy(host,hosttmp,portLoc);
            host[portLoc]='\0';

            
            char gotHost[HOST_NAME_MAX];
            gethostname(gotHost,HOST_NAME_MAX);

            printf("*****Host Info*****\nReceived Host: %s\nActual Host: %s\n",host,gotHost);

            /*Creates variants on hostname received and gethostname host with dcs.gla.ac.uk at the end.*/
            char DCSgotHost[HOST_NAME_MAX];
            sprintf(DCSgotHost,"%s.dcs.gla.ac.uk",gotHost);
            char DCShost[HOST_NAME_MAX];
            sprintf(DCShost,"%s.dcs.gla.ac.uk",host);

            /*Check that GET starts*/
            if(strncmp(get,"GET",3)!=0){
                printf("Did not receive initial GET request!\n");
                writeBadRequest(connfd,protocol);
            }
            /*Check received host against result from gethostname(). Also checks variations using dcs.gla.ac.uk*/
            else if((strcmp(gotHost,host)!=0)&&
                   ((strcmp(DCSgotHost,host)!=0)&&
                   (strcmp(gotHost,DCShost)!=0))){
                writeBadRequest(connfd,protocol);
                printf("Hostname Mismatch encountered!\n");             
            }
            else{
                /*Assuming nothing else goes wrong, write the requested page, and reset the buffer for further reads.*/
                writeRequested(connfd,protocol,url);
                totalRead=0;
                memset(buf,0,sizeof(buf));
                }
        } 
    }
    /*Frees the buffer, closes the connection, frees the allocated space for the connection value. Returns null as required for void*. */
    free(buf);
    close(connfd);
    free(voidconn);
    return NULL;
}

int main(){
    /*Declares the base file descriptor, and the addresses.*/    
    int fd;
    struct sockaddr_in addr;
    struct sockaddr_in cliaddr;
    socklen_t cliaddrlen = sizeof(cliaddr);

    /*Create file descriptor, define socket, bind socket, listen on fd, create new connection fd, read incoming request into buffer buf.*/
    fd = initialiseFd();
    addr = defineSocket(addr,8080);
    bindSocket(fd,addr);
    listenFd(fd,4);

    while(1){
        int* connfd = malloc(sizeof(int));
        pthread_t thrd;
        *connfd = initialiseConnFd(fd,cliaddr,cliaddrlen);
        pthread_create(&thrd,NULL,readRequest,(void*)connfd);
    }    

    /*close fd*/
    close(fd);
    return 0;

}
