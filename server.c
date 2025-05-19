#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include "Md5.c"

/*

SERVER

*/

struct ClientData {

    int sockfd;
    int clientId;

};

sem_t x;

char openfiles[25][25];

int existinfiles(char *filename) {

    sem_wait(&x);

    for (int i = 0; i < 25; i++) {
        if (strcmp(filename, openfiles[i]) == 0) {
            sem_post(&x);
            return -2;
        }
    }

    sem_post(&x);

    return 0;
}

int removefiles(char *filename) {

    sem_wait(&x);

    for (int i = 0; i < 25; i++) {
        if (strcmp(filename, openfiles[i]) == 0) {
            strcpy(openfiles[i], "");
            break;
        }
    }

    sem_post(&x);
}

int addfile(char *filename) {

    sem_wait(&x);

    for (int i = 0; i < 25; i++) {
        if (strcmp(filename, openfiles[i]) == 0) {
            sem_post(&x);
            return -2;
        }
    }

    for (int k = 0; k < 25; k++) {
        if (strcmp(openfiles[k], "") == 0) {
            strcpy(openfiles[k], filename);
            sem_post(&x);
            return k;
        }
    }

    sem_post(&x);
}

/* file name, file descriptor */
FILE *openappend(char *filename) {

    FILE *fd;

    fd = fopen(filename, "a");

    addfile(filename);

    return fd;


}

/* file name, file descriptor */
FILE* openread(char *filename) {

    FILE* fd;
    
    fd = fopen(filename,"r");

    return fd;

}

void *newclient(void *clientdata) {

    size_t n;
    char buffer[255];
    char message[255];
    FILE *currentfd;
    int opened = 0;
    int readcnt = 0;

    struct ClientData cd = *(struct ClientData *) clientdata;

    int sockfd = cd.sockfd;
    int clientId = cd.clientId;

    while((n = read(sockfd, buffer, sizeof(buffer))) != 0) {

        printf("%s", buffer);

        int exist;

        /* command splitter */
        char cmd[25][25];
        char *token;

        token = strtok(buffer, " \n");

        int i = 0;
        while (token != NULL) {
            strcpy(cmd[i], token);
            token = strtok(NULL, " \n");
            i++;
        }

        memset(message, '\0', sizeof(message));
        memset(buffer, '\0', sizeof(buffer));

        /* commands */
        if (strcmp(cmd[0], "openRead") == 0) {

            exist = existinfiles(cmd[1]);

            if (opened == 0 && exist != -2) {

                opened = 1;
                currentfd = openread(cmd[1]);

                write(sockfd, message, sizeof(message));

            }

            else if (opened == 1) {

                sprintf(message, "A file is already open for reading\n");
                write(sockfd, message, sizeof(message));          

            }

            else if (exist == -2) {

                printf("The file is opened by another client\n");
                sprintf(message, "The file is opened by another client\n");
                write(sockfd, message, sizeof(message));     

            }

        }

        else if (strcmp(cmd[0], "openAppend") == 0) {

            exist = existinfiles(cmd[1]);

            if (exist == 0 && opened == 0) {

                opened = 1;
                currentfd = openappend(cmd[1]);

                write(sockfd, message, sizeof(message));
            }

            else if (opened == 1) {

                printf("A file is already open for appending\n");
                sprintf(message, "A file is already open for appending\n");
                write(sockfd, message, sizeof(message));            

            }

            else if (exist == -2) {

                printf("The file is opened by another client\n");
                sprintf(message, "The file is opened by another client\n");
                write(sockfd, message, sizeof(message));                       

            }

        }

        else if (strcmp(cmd[0], "read") == 0) {

            if (opened == 1) {

                int c = 0;
                int i = 0;
                int k = 0;

                char temp[255];

                while ((c = fgetc(currentfd)) != EOF) {
                    temp[i++] = c;
                }

                temp[i] = '\n';

                for (int j = readcnt; j < readcnt + atoi(cmd[1]); j++) {
                    message[k++] = temp[j];
                }

                readcnt = readcnt + atoi(cmd[1]);

                message[k] = '\n';

                write(sockfd, message, sizeof(message));

            }

            else {

                printf("File not open\n");
                sprintf(message, "File not open\n");
                write(sockfd, message, sizeof(message));

            }

        }

        else if (strcmp(cmd[0], "append") == 0) {

            if (opened == 1) {

                fprintf(currentfd, "%s", cmd[1]);
                write(sockfd, message, sizeof(message));  

            }
            else {

                printf("File not open\n");
                sprintf(message, "File not open\n");
                write(sockfd, message, sizeof(message));                

            }

        }

        else if (strcmp(cmd[0], "close") == 0) {

            fclose(currentfd);
            removefiles(cmd[1]);
            opened = 0;
            readcnt = 0;

            write(sockfd, message, sizeof(message));                    

        }

        else if (strcmp(cmd[0], "getHash") == 0) {

            FILE *inFile = fopen (cmd[1], "r");

            if (inFile != NULL) {

                fclose(inFile);

                if (existinfiles(cmd[1]) != -2) {

                    unsigned char digest[16];
                    MDFile(cmd[1], digest);

                    unsigned char * temp = digest;

                    unsigned char tempmsg[200];

                    int i = 0;
                
                    while(*temp)
                    {
                        sprintf(tempmsg + i*2, "%02x", *temp);

                        temp++;

                        i++;
                    }

                    sprintf(tempmsg + i*2, "%s", "\n");

                    write(sockfd, tempmsg, sizeof(tempmsg));

                } else {

                    printf("File is already open for appending\n");
                    sprintf(message, "File is already open for appending\n");
                    write(sockfd, message, sizeof(message));

                }

            } else {

                printf("File not found\n");
                sprintf(message, "File not found\n");
                write(sockfd, message, sizeof(message));

            }

        }

        else if (strcmp(cmd[0], "quit") == 0) {

            close(sockfd);
            pthread_exit(NULL);

            break;
        }
    }

    close(sockfd);
    pthread_exit(NULL);

}

int main(int argc, char* argv[]) {

    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr;
    struct sockaddr_storage serverStorage;

    socklen_t addr_size;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(atoi(argv[1]));

    bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    if (listen(serverSocket, 4) == 0) {
        printf("server started\n");
    } else {
        printf("error\n");
    }

    pthread_t clientThreads[4];
    sem_init(&x, 0, 1);

    int i = 0;

    while (1) {
        addr_size = sizeof(serverStorage);

        clientSocket = accept(serverSocket, (struct sockaddr*)&serverStorage, &addr_size);

        struct ClientData clientdata = {clientSocket, i};

        if (pthread_create(&clientThreads[i++], NULL, newclient, &clientdata) != 0) {
            printf("client failed to connect\n");
        }
    }

}
