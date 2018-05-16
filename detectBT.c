//
//  main.c
//  detectBT.c
//
//  Created by Josh Randall on 3/26/17.
//  Copyright © 2017 Josh Randall. All rights reserved.
//

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <wiringPi.h>

const int pwmPin = 23;

void lockDoor();
void unlockDoor();
static int isConnected(int socket, int BT_ID, long bdaddr);
void set_cr_echo_mode();
int tty_mode(int how);
void logEntry();

int logFile, BTsocket, connSocket, previous=1;

int main(int argc, const char * argv[]) {
    
    void quitProgram(int);
    signal( SIGINT, quitProgram);                 // install the handler
    
    struct hci_conn_info_req *connReq;
    struct sockaddr_rc addr = {0};
    int8_t rssi;
    bdaddr_t bdaddr;
    char phone[19] = "D0:25:98:B5:B0:E4"; // Bluetooth unique ID of Device to be detected.
    char ctrl[2];
    int BT_ID, connStatus = -1;
    int found = -1, unlock=0, lock =0, i=0;
    int ctrlFile;
    
    str2ba(phone, &bdaddr);
    
    wiringPiSetupGpio(); // initialize
    
    pinMode(pwmPin, OUTPUT);
    digitalWrite(pwmPin, HIGH); // system is locked initially.
    
    // Create/open a log file to show logged entries
    if ((logFile = open("./Desktop/log.txt", O_RDWR|O_APPEND|O_CREAT, S_IROTH|S_IWOTH|S_IXOTH)) == -1){
        perror("Could not open or create file");
        exit(1);
    }
    
    // we are now going to start the control server;
    pid_t fprogram = fork();
    
    if (fprogram==0){
        execv("./detectBTServer", (char * const*)"");
    }
    
    // Now our main program
    // pings every 10 seconds
    while (1){
        
        str2ba(phone, &bdaddr);
        BT_ID = hci_get_route(NULL);
        BTsocket = hci_open_dev( BT_ID );
        if (BT_ID < 0 || BTsocket < 0){
            perror("Bluetooth device or Socket is not able to open");
            exit(1);
        }
        
        found = isConnected(BTsocket, BT_ID, (long) &bdaddr);
        //printf("Found: %d\n", found); // DEBUGGING
        
        if (found==1){
            printf("Device is connected!\n");
            connStatus=0;
        }
        
        // code below pings a connection to phone if phone is not already connected
        while(connStatus!=0){
            
            // Create/open a control file to get remote access input
            if ((ctrlFile = open("./Desktop/control.txt", O_RDWR|O_CREAT, S_IROTH|S_IWOTH|S_IXOTH)) == -1){
                perror("Could not open or control file");
                exit(1);
            }
            
            memset(ctrl, '\0', sizeof(ctrl));
            if (read(ctrlFile, ctrl, 1)>0){
                printf("ctrlFile: %s\n", ctrl);
                if (strcmp(ctrl, "0")==0){
                    printf("Remote access, set system to UNLOCK\n");
                    logEntry();
                    unlockDoor();
                    sleep(30);
                    printf("Remote access time up! Setting system to LOCK\n");
                    logEntry();
                    lockDoor();
                    remove("./Desktop/control.txt");
                }
                memset(ctrl, '\0', sizeof(ctrl));
            }
            
            // allocate the bluetooth connection socket
            connSocket = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
            
            // these are the connection parameters to connect to Device
            addr.rc_family = AF_BLUETOOTH;
            addr.rc_channel = (uint8_t) 1;
            str2ba(phone, &addr.rc_bdaddr);
            
            // connect to Device
            connStatus = connect(connSocket, (struct sockaddr *)&addr, sizeof(addr));
            
            // print status
            if (connStatus == 0) {
                printf("Connection to Device successful!\n");
                found = 1;
            }
            else if (connStatus<0){
                printf("Cannot find Device system is 'LOCKED' (Press ctrl+c to terminate program)\n");
            }
            
            // if connection not found close socket then infinitely loop every 10s
            // until connection is found
            if (connStatus!=0){
                struct sockaddr_rc addr = {0};
                close(connSocket);
                sleep(10);
            }
        }
        
        // once connection is established perform actions based on RSSI value (proximity)
        if (connStatus==0 && found==1){
            
            // find RSSI value 5 times for more accurate proximity reading
            for(i=0;i<5;i++){
                
                connReq = malloc(sizeof(*connReq) + sizeof(struct hci_conn_info));
                if(!connReq){
                    perror("Cannot allocate memory for connection request pointer");
                    exit(1);
                }
                
                if (found!=1)
                    bacpy(&connReq->bdaddr, &addr.rc_bdaddr);
                else if (found == 1)
                    bacpy(&connReq->bdaddr, &bdaddr);
                connReq->type = ACL_LINK;
                
                // get RSSI value if phone is connected. if sudden disconnect Lock system.
                if (isConnected(BTsocket, BT_ID, (long) &bdaddr)){
                    if (ioctl(BTsocket, HCIGETCONNINFO, (unsigned long) connReq)<0){
                        perror("Failed to obtain connection info");
                        exit(1);
                    }
                    
                    if (hci_read_rssi(BTsocket, htobs(connReq->conn_info->handle), &rssi, 1000)<0){
                        perror("Failed to read RSSI value");
                        exit(1);
                    }
                    printf("RSSI %d: %d\n", i+1, rssi);
                    if (rssi>=0){
                        unlock++;
                    }
                    else lock++;
                }
                else{
                    if (previous == 0){
                        printf("Device disconnected! Lock set to 'LOCK'\n");
                        // Write Log
                        logEntry();
                        // lock door
                        lockDoor();
                    }
                    else
                        printf("Device disconnected! System is 'LOCKED'\n");
                    connStatus = -1;
                    previous = 1;
                    i=4;
                }
                
                free(connReq);
                usleep(500000); // 0.5 second delay 5 times for total of 2.5s
            }
            
            // perform action based on bluetooth sensor proximity
            if (isConnected(BTsocket, BT_ID, (long) &bdaddr)){
                if (unlock>lock){
                    if (previous == 1){
                        printf("Device in range! Mode set to 'UNLOCK'\n");
                        // Write Log
                        logEntry();
                        // unlock door
                        unlockDoor();
                    }
                    else printf("Standing by... System is 'UNLOCKED' (Press ctrl+c to terminate program)\n");
                    unlock=0;
                    lock=0;
                    previous = 0;
                }
                else{
                    if (previous == 0){
                        printf("Device out of range, lock set to 'LOCK'\n");
                        // Write Log
                        logEntry();
                        // lockdoor
                        lockDoor();
                    }
                    else printf("Standing by... System is 'LOCKED' (Press ctrl+c to terminate program)\n");
                    unlock=0;
                    lock=0;
                    previous = 1;
                }
            }
            else{
                if (previous == 0){
                    printf("Device disconnected! Lock set to 'LOCK'\n");
                    // Write Log
                    logEntry();
                    // Lock door
                    lockDoor();
                }
                else printf("Device disconnected! System is 'LOCKED'\n");
                connStatus = -1;
                previous = 1;
            }
            
        }
    }
    
    return 0;
}

void lockDoor(){
    
    digitalWrite(pwmPin, HIGH);
    
    if(write(logFile, " system was set to LOCK\n", 24)!= 24){
        perror("Cannot write log\n");
        exit(1);
    }
    
}

void unlockDoor(){
    
    digitalWrite(pwmPin, LOW);
    
    if(write(logFile, " system was set to UNLOCK\n", 26)!= 26){
        perror("Cannot write log\n");
        exit(1);
    }
    
}

static int isConnected(int socket, int BT_ID, long bdaddr){
    
    struct hci_conn_list_req *connList;
    struct hci_conn_info     *connInfo;
    int k;
    
    if (!(connList = malloc(10 * sizeof(*connInfo) + sizeof(*connList)))) {
        perror("Failed to allocate memory for connection list");
        exit(1);
    }
    
    connList->dev_id = BT_ID;
    connList->conn_num = 10;
    connInfo = connList->conn_info;
    
    if (ioctl(socket, HCIGETCONNLIST, (void *) connList)) {
        perror("Failed to gather connection list");
        exit(1);
    }
    
    // Compare bdaddr to connInfo bdaddr if it matches that bdaddr is connected!
    for (k=0; k<connList->conn_num; k++, connInfo++)
        if (!bacmp((bdaddr_t *) bdaddr, &connInfo->bdaddr)){
            free(connList);
            return 1;
        }
    
    free(connList);
    return 0;   // returns 0 if bdaddr is not in connInfo (not connected)
    
}

void quitProgram(int singnum){
    
    char input;
    
    tty_mode(0);
    set_cr_echo_mode();
    
    printf("Would you like to quit? [y/n] ");
    input = getchar();
    tty_mode(1);
    
    if ( input == 'y' || input == 'Y'){
        if (previous==0){
            printf("\nSystem reverting to default LOCK position\n");
            logEntry();
            lockDoor();
        }
        //printf("\nClosing bluetooth connection...\n");
        close(connSocket);
        close(BTsocket);
        printf("\nClosing application...\n");
        close(logFile);
        exit(0);
    }
    putchar('\n');
}

void set_cr_echo_mode(){
    struct termios ttystate, newstate;
    
    tcgetattr( 0, &ttystate);               // read current setting
    ttystate.c_lflag    &= ~ICANON;          // No buffering
    ttystate.c_lflag    |= ECHO;            // Enable echo
    ttystate.c_cc[VMIN] = 1;                // get 1 char at a time
    tcsetattr( 0, TCSANOW, &ttystate);      // install settings
}

int tty_mode(int how){
    static struct termios original_mode;
    static int            original_flags;
    static int            stored=0;
    
    if (how==0){
        tcgetattr(0, &original_mode);
        original_flags = fcntl(0, F_GETFL);
        stored = 1;
    }
    else if (stored){
        tcsetattr(0, TCSANOW, &original_mode);
        fcntl(0, F_SETFL, original_flags);
    }
    return 0;
}

void logEntry(){
    char timebuf[25];
    time_t currentTime;
    
    time(&currentTime);
    strcpy(timebuf, ctime(&currentTime));
    if(write(logFile, timebuf, sizeof(timebuf))!= sizeof(timebuf)){
        perror("Cannot write log\n");
        exit(1);
    }
    memset(&timebuf[0], 0, sizeof(timebuf));
}


