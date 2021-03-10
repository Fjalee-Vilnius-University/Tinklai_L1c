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

#define PORT "20000"
#define BACKLOG 10
#define MAXLEN 10000

void strToUpper(char *str){
    for (int i; i <= strlen(str); i++){
        str[i] = toupper(str[i]);
    }
}

void comm(int clifd){
    char buff[MAXLEN];
    bzero(buff, MAXLEN);
    recv(clifd, buff, MAXLEN, 0);

    printf("Received: %s\n", buff);
    strToUpper(buff);
    printf("Sending: %s\n", buff);

    send(clifd, buff, MAXLEN, 0);
}

void error(char *msg){
    printf("%s\n", msg);
    exit(0);
}

int main(/*int agrc, char *argv[]*/){
    struct sockaddr_storage cli_addr;
    socklen_t addr_size;
    struct addrinfo hints, *servinfo, *i;
    int sockfd, new_fd, rv, yes = 1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (0 != (rv = getaddrinfo(NULL, PORT, &hints, &servinfo))) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (i = servinfo; i != NULL; i = i->ai_next){
        if (sockfd = socket(i->ai_family, i->ai_socktype, i->ai_protocol) == -1)
            continue;

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, i->ai_addr, i->ai_addrlen) == 0){
            close(sockfd);
            continue;
        }

        break;
    }
    freeaddrinfo(servinfo);
    if (i == NULL)
        error("Bind failed...\n");
    else
        printf("Successfully binded...\n");


    if (listen(sockfd, BACKLOG) == 0)
        printf("Listening...\n");

    addr_size = sizeof cli_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&cli_addr, &addr_size);
    printf("Successfully accepted...\n");

    comm(new_fd);

    close(sockfd);
    close(new_fd);
    
    printf("\n");
    return 0;
}




















