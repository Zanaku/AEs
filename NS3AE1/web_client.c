#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

int main(int argc, char * argv[]){
    int fd;

    struct addrinfo   hints, *ai, *ai0;
    int i;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family    = PF_UNSPEC;
    hints.ai_socktype  = SOCK_STREAM;

    if(argc!=2){
        printf("GET OUT");
        return 0;
    }

    if ((i = getaddrinfo(argv[1], "5001", &hints, &ai0)) != 0) {
        printf("Unable to look up IP address: %s", gai_strerror(i));

    }

    for (ai = ai0; ai != NULL; ai = ai->ai_next) {
        fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd == -1) {
            perror("Unable to create socket");
            continue;
        }
        if (connect(fd, ai->ai_addr, ai->ai_addrlen) == -1) {
            perror("Unable to connect");
            close(fd);
            continue;
        }
        int k;
        char data[] = "DATE\r\n";
        int datalen = strlen(data);
        for (k=0;k<10;k++){
            if(write(fd, data, datalen) == -1){
                printf("HELP");
            }
            ssize_t j;
            ssize_t rcount;
            char buf[512];
            rcount = read(fd, buf, 512);
                for (j = 0; j < rcount; j++){
                    printf("%c", buf[j]);
                }
            
            sleep(2);
        }
        break;
    }

    close(fd);

    return 0;
}
