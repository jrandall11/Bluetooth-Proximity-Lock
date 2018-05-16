//
//  main.c
//  clientForLockSystem
//
//  Created by Josh Randall on 5/1/17.
//  Copyright © 2017 Josh Randall. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <strings.h>

#define HOSTLEN 256
#define BACKLOG 1

int connect_to_server(char *host, int portnum);
void talk_with_server(int fd);

int main(int argc, const char * argv[]) {
    printf("welcome\n");
    int fd;
    
    fd = connect_to_server("169.254.171.147", 5236);
    if (fd==-1){
        printf("Cannot connect to server\n");
        exit(1);
    }
    talk_with_server(fd);
    
    return 0;
}

int connect_to_server(char *host, int portnum){
    
    int sock;
    struct sockaddr_in servadd;         // the number to call
    struct hostent      *hp;            // used to get number
    
    // step 1 get socket
    sock = socket(AF_INET, SOCK_STREAM, 0);     // get a line
    if (sock==-1)
        return -1;
    
    // step 2 connect to server
    bzero( &servadd, sizeof(servadd) );     // zero the address
    hp = gethostbyname(host);               // lookup host's ip #
    if (hp == NULL)
        return -1;
    bcopy(hp->h_addr, (struct sockaddr *)&servadd.sin_addr, hp->h_length);
    servadd.sin_port = htons (portnum);     // fill in port number
    servadd.sin_family = AF_INET;           // fill in socket type
    
    if (connect(sock, (struct sockaddr *)&servadd, sizeof(servadd))!=0)
        return -1;
    
    return sock;
}

void talk_with_server(int fd){
    char message[BUFSIZ];
    char log[1024];
    FILE *login;
    int c;
    long li;
    printf("What would you like to do?\n\tG- Get Log\n\tU - Unlock System\n\tQ - Quit Client\n");
    while((c = getchar())){
        if (c=='g'||c=='G'){
            printf("Getting Log...");
            strcpy(message, "Get Log");
            if ( write(fd, message, BUFSIZ)==-1)
                printf("Cannot write\n");
            
            memset(log, '\0', sizeof(log));
            sleep(1);
            li = recv(fd, log, sizeof(log), 0);
            printf("li = %d\n", (int)li);
            if(li<=0){
                printf("Connection may be closed or there is something wrong");
                exit(1);
            }
            printf("%s\n", log);
            close(fd);
            
        }
        else if (c=='u'||c=='L'){
            printf("unlocking system...\n");
            strcpy(message, "Unlock System");
            if ( write(fd, message, BUFSIZ)==-1)
                printf("Cannot write\n");
            
        }
        else if (c=='q'||c=='Q'){
            close(fd);
            exit(0);
        }
        
        memset(message, '\0', sizeof(message));
        printf("What would you like to do?\n\tG- Get Log\n\tU - Unlock System\n\tQ - Quit Client\n");
        close(fd);
        fd = connect_to_server("169.254.171.147", 5236);
        if (fd==-1){
            printf("Cannot connect to server\n");
            exit(1);
        }
    }
}

