#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

/*

CLIENT

*/

char cmd[5][10];
char buffer[255];

void getinput() {

    char str[80];
    char *token;

    printf("> ");
    fgets(str, sizeof(str), stdin);
    token = strtok(str, " \n");

    int i = 0;
    while (token != NULL) {
        strcpy(cmd[i], token);
        token = strtok(NULL, " \n");
        i++;
    }
}

int connect2server(char* ipaddress, u_int16_t portnumber) {

    if (strcmp(ipaddress, "localhost") == 0) {
        strcpy(ipaddress, "127.0.0.1");
    }

    int sockD = socket(AF_INET, SOCK_STREAM, 0);
  
    struct sockaddr_in servAddr;
  
    servAddr.sin_family = AF_INET;
    servAddr.sin_port
        = htons(portnumber); // use some unused port number
    servAddr.sin_addr.s_addr = INADDR_ANY;

    if (inet_pton(AF_INET, ipaddress, &servAddr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
  
    int connectStatus
        = connect(sockD, (struct sockaddr*)&servAddr,
                  sizeof(servAddr));
  
    if (connectStatus == -1) {
        printf("Error...\n");
        return -1;
    }
  
    else {
        
        return sockD;

    }
}

int main(int argc, char* argv[]) {

    int sockfd;
    char buffer[255];

    sockfd = connect2server(argv[1], atoi(argv[2]));

    printf("> ");

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {

        if (strcmp(buffer, "quit\n") == 0) {
            break;
        } else {
            write(sockfd, buffer, strlen(buffer));
            read(sockfd, buffer, sizeof(buffer));
            fputs(buffer, stdout);
            printf("> ");
        }
    }

    close(sockfd);
    exit(0);
}