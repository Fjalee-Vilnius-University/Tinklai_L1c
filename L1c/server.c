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

#define PORT "20000"
#define BACKLOG 10
#define MAXLEN 10000

void sigchld_handler(int s)
{
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

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

int main(void/*int agrc, char *argv[]*/){
    struct sockaddr_storage cli_addr;
    socklen_t addr_size;
    struct addrinfo hints, *servinfo, *i;
    int sockfd, new_fd, rv, yes = 1;
    struct sigaction sa;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    //hints.ai_family = AF_INET6;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (0 != (rv = getaddrinfo(NULL, PORT, &hints, &servinfo))) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (i = servinfo; i != NULL; i = i->ai_next){
        if (-1 == (sockfd = socket(i->ai_family, i->ai_socktype, i->ai_protocol)))
            continue;

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, i->ai_addr, i->ai_addrlen) == -1){
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

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        error("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1){
        addr_size = sizeof cli_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&cli_addr, &addr_size);
        printf("Successfully accepted...\n");
        
        inet_ntop(cli_addr.ss_family, 
            get_in_addr((struct sockaddr *)&cli_addr), 
            s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()){
            close(sockfd);
            if (-1 == send(new_fd, "Connected to the server", 13, 0))
                error("Sending failed...");
            close(new_fd);
            exit(0);
        }
        close(new_fd);
    }

    //comm(new_fd);

    printf("\n");
    return 0;
}




















