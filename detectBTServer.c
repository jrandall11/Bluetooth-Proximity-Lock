#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <strings.h>
#include <string.h>

#define HOSTLEN 256
#define BACKLOG 1

int make_server_socket(int portnum);
int make_server_socket_q(int portnum, int backlog);

int main(int argc, const char * argv[]){
    int sock, fd, ctrlFile, lf;
    char request[BUFSIZ];
    char logfile[1024];
    FILE *fpin, *log, *logout;
    
    // Create/open a control file to show logged entries
    if ((ctrlFile = open("./Desktop/control.txt", O_RDWR|O_CREAT, S_IRWXG|S_IRWXU|S_IRWXO)) == -1){
        perror("Could not open or create file");
        exit(1);
    }
    
    fflush(stdout);
    printf("setting up...\n");
    sock = make_server_socket(5236);
    if (fd==-1){
        printf("Cannot make server socket\n");
        exit(1);
    }
    printf("listening for connections\n");
    while (1){
        fd = accept(sock, NULL, NULL);
        if (fd==-1)
            break;
        fpin = fdopen(fd, "r");
        fgets(request, BUFSIZ, fpin);
        if (strcmp(request, "Unlock System")==0){
            printf("I will %s\n", request);
            if(write(ctrlFile, "0", 1)!= 1){
                perror("Cannot write ctrlfile\n");
                exit(1);
            }
        }
        else if (strcmp(request, "Lock System")==0){
            printf("I will %s\n", request);
            if(write(ctrlFile, "1", 1)!= 1){
                perror("Cannot write ctrlfile\n");
                exit(1);
            }
        }
        if (strcmp(request, "Get Log")==0){
            printf("I will %s\n", request);
            // Create/open a log file to show logged entries
            if ((lf = open("./Desktop/log.txt", O_RDONLY)) == -1){
                perror("Could not open or create file");
                exit(1);
            }
            log = fdopen(lf, "r");
            fgets(logfile, 1024, log);
            logout = fdopen(fd, "w");
            fprintf(logout, logfile);
	    fclose(logout);
            
        }
        //remove("./Desktop/control.txt");
        if ((ctrlFile = open("./Desktop/control.txt", O_RDWR|O_CREAT, S_IRWXG|S_IRWXU|S_IRWXO)) == -1){
            perror("Could not open or create file");
            exit(1);
        }
        memset(request, '\0', sizeof(BUFSIZ));
        close(fd);
    }
    return 0;
}

int make_server_socket(int portnum){
    return make_server_socket_q(portnum, BACKLOG);
}
int make_server_socket_q(int portnum, int backlog){
    struct sockaddr_in saddr;   // build our address here
    int sock_id;
    
    sock_id = socket(PF_INET, SOCK_STREAM, 0); //get a socket
    if (sock_id==-1)
        return -1;
    
    // build address and bind it to socket
    bzero((void *)&saddr, sizeof(saddr));   // clear out struct
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(portnum);        // fill in socket port
    saddr.sin_family = AF_INET;             // fill in addr family
    if (bind(sock_id, (struct sockaddr *)&saddr, sizeof(saddr))!=0)
        return -1;
    
    // arrange for incoming calls
    if (listen(sock_id, backlog)!=0)
        return -1;
    
    return sock_id;
}
