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

int get_listener_socket(void)
{
    int listener;
    int yes=1;
    int rv;

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6;
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

void add_to_pfds(struct pollfd *pfds[], int newfd, int *fd_count, int *fd_size)
{
    // Double pdfs space if its full
    if (*fd_count == *fd_size) {
        *fd_size *= 2;

        *pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
    }

    (*pfds)[*fd_count].fd = newfd;
    (*pfds)[*fd_count].events = POLLIN;

    (*fd_count)++;
}

void del_from_pfds(struct pollfd pfds[], int i, int *fd_count)
{
    // Copy the one from the end over this one
    pfds[i] = pfds[*fd_count-1];

    (*fd_count)--;
}

/*void strToUpper(char *str){
    for (int i; i <= strlen(str); i++){
        str[i] = toupper(str[i]);
    }
}*/

int main(void){
    int listener;
    int new_fd;
    struct sockaddr_storage cli_addr;
    socklen_t addr_size;
    char buff[MAXLEN];
    char remoteIP[INET6_ADDRSTRLEN];

    // Start off with room for 5 connections
    // (We'll realloc as necessary)
    int fd_count = 0;
    int fd_size = 5;
    struct pollfd *pfds = malloc(sizeof *pfds * fd_size);

    listener = get_listener_socket();
    pfds[0].fd = listener;
    pfds[0].events = POLLIN;
    fd_count = 1;

    for(;;) {
        int poll_count = poll(pfds, fd_count, -1);

        if (poll_count == -1)
            error("Failed creating poll...\n");

        // Run through the existing connections looking for data to read
        for(int i = 0; i < fd_count; i++) {
            if (pfds[i].revents & POLLIN) {

                if (pfds[i].fd == listener) { // If listener is ready to read, handle new connection
                
                    addr_size = sizeof cli_addr;
                    new_fd = accept(listener, (struct sockaddr *)&cli_addr, &addr_size);

                    if (new_fd == -1) 
                        error("Acepting failed ...\n");
                    else {
                        add_to_pfds(&pfds, new_fd, &fd_count, &fd_size);

                        printf("pollserver: new connection from %s on "
                            "socket %d\n",
                            inet_ntop(cli_addr.ss_family,
                                get_in_addr((struct sockaddr*)&cli_addr),
                                remoteIP, INET6_ADDRSTRLEN),
                            new_fd);
                    }
                }

                else {    // If not the listener, we're just a regular client
                    int nbytes = recv(pfds[i].fd, buff, sizeof buff, 0);
                    int sender_fd = pfds[i].fd;

                    if (nbytes <= 0) {    // Got error or connection closed by client

                        // Connection closed
                        if (nbytes == 0)    
                            printf("pollserver: socket %d hung up\n", sender_fd);
                        else 
                            error("Error receiving...\n");

                        close(pfds[i].fd);
                        del_from_pfds(pfds, i, &fd_count);
                    } 
                    else {
                        for(int j = 0; j < fd_count; j++) {    // Send to everyone
                            int dest_fd = pfds[j].fd;

                            // Except the listener and ourselves
                            if (dest_fd != listener && dest_fd != sender_fd) {
                                if (send(dest_fd, buff, nbytes, 0) == -1) {
                                    error("Error sending...\n");
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




















