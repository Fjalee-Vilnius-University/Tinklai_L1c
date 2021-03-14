#include <stdio.h> 
#include <ctype.h>
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <poll.h>

#define PORT "20000"
#define BACKLOG 10
#define MAXLEN 256

void error(char *msg){
    printf("%s\n", msg);
    exit(1);
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int get_listener_socket(void)
{
    int listener;
    int yes=1;
    int rv;

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (0 != (rv = getaddrinfo(NULL, PORT, &hints, &servinfo))) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }
    
    for(p = servinfo; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) { 
            continue;
        }
        
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    if (p == NULL)
        error("Bind failed...\n");
    else
        printf("Successfully binded...\n");
    freeaddrinfo(servinfo);

    if (listen(listener, 10) == 0) {
        printf("Listening...\n");
    }
    else 
        error("Failed listening...\n");

    return listener;
}

int sendall(int s, char *buf, int *len)
{
    int total = 0;
    int bytesleft = *len;
    int n;

    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total;

    return n==-1?-1:0; // return -1 on failure, 0 on success
}

int main(void){
    int listener;
    int new_fd;
    struct sockaddr_storage cli_addr;
    socklen_t addr_size;
    char buff[MAXLEN];
    int nbytes;
    char remoteIP[INET6_ADDRSTRLEN];

    fd_set master;
    fd_set read_fds;
    int fdmax;

    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    listener = get_listener_socket();
    FD_SET(listener, &master);
    fdmax = listener;

    char userNames[100][20];
    int totalUsers = 0;

    for(;;) {
        read_fds = master;
        if(-1 == select(fdmax+1, &read_fds, NULL, NULL, NULL))
            error("Failed selector...");

        // Run through the existing connections looking for data to read
        for(int i = 0; i <= fdmax; i++) {
            if(FD_ISSET(i, &read_fds)){
                if(i == listener){
                    addr_size = sizeof cli_addr;
                    new_fd = accept(listener, (struct sockaddr *)&cli_addr, &addr_size);

                    if (new_fd == -1) 
                        error("Accepting failed ...\n");
                        
                    else {
                        FD_SET(new_fd, &master);
                        if(new_fd > fdmax)
                            fdmax = new_fd;

                        if (send(new_fd, "Enter your name: ", 17, 0) == -1) {
                            error("Error sending...\n");
                        }
                        
                        memset(buff, 0, strlen(buff));
                        int nbytes = recv(new_fd, buff, sizeof buff, 0);

                        int j;
                        for(j = 0; buff[j] != '\r'; j++){
                            userNames[new_fd][j] = buff[j];
                        }
                        userNames[new_fd][j] ='\0';

                        // printf("New connection from %s on "
                        //     "socket %d\n",
                        //     inet_ntop(cli_addr.ss_family,
                        //         get_in_addr((struct sockaddr*)&cli_addr),
                        //         remoteIP, INET6_ADDRSTRLEN),
                        //     new_fd);
                    }
                }

                else {    // If not the listener, we're just a regular client
                    memset(buff, 0, strlen(buff));
                    nbytes = recv(i, buff, sizeof buff, 0);

                    if (nbytes <= 0) {    // Got error or connection closed by client
                        // Connection closed
                        if (nbytes == 0){
                            printf("pollserver: socket %d hung up\n", i);
                        }
                        else 
                            error("Error receiving...\n");

                        close(i);
                        FD_CLR(i, &master);
                    } 
                    else {
                        for(int j = 0; j <= fdmax; j++) {    // Send to everyone
                            if(FD_ISSET(j, &master)){
                                // Except the listener
                                char strToPrint[MAXLEN];
                                if (j != listener) {
                                    char nameWBuff[nbytes+20];

                                    strcpy(nameWBuff, userNames[i]);
                                    strcat(nameWBuff, ": ");
                                    strcat(nameWBuff, buff);

                                    int msgLen = strlen(nameWBuff);
                                    if (sendall(j, nameWBuff, &msgLen) == -1) {
                                        error("Error sending...\n");
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}