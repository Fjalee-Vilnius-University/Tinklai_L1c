#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>
#include "ll.h"

#define PORT "22222"
#define BACKLOG 10

struct data{
    int descriptor;
    char* name;
};

int compare_descriptors(void* data1, void* data2){
    return ((struct data*)data1)->descriptor == ((struct data*)data2)->descriptor;
}

int compare_name(void* data1, void* data2){
    if(!((struct data*)data1)->name || !((struct data*)data2)->name){
        return 0;
    }
    return strcmp(((struct data*)data1)->name, ((struct data*)data2)->name) == 0;
}

void add_connection_to_selector(int fd, int* fdmax, fd_set* set, struct node** head){
    if(send(fd, "ATSIUSKVARDA\n", 13, 0) == -1){
        fprintf(stderr, "Failed to send a message");    
        close(fd);
    }else{
        FD_SET(fd, set);
        
        *fdmax = fd > *fdmax ? fd : *fdmax;
        
        struct data* data = malloc(sizeof(struct data));
        
        data -> descriptor = fd;
        data -> name = NULL;
        push_node(head, data);
        
    }
}

void find_new_max_fd(struct node *node, int* fdmax){
    int max=0;
    struct data* data;
    while(node != NULL){
        data = (struct data*) node->data;
        max = data->descriptor > max ? data->descriptor : max;
        node = node->next_node;
    }
}

void remove_socket(int fd, int* fdmax, fd_set* set, struct node** head){
    close(fd);
    FD_CLR(fd, set);
    
    struct data data;
    data.descriptor = fd;

    remove_node(head, &data, compare_descriptors);

    if(*fdmax == fd){
        find_new_max_fd(*head, fdmax);
    }
}

int string_to_command_number(const char* str){
    if(!strncmp(str, "USERNAME", 8)) return 1;
    if(!strncmp(str, "PRANESIMAS", 3)) return 2;
    return 0;
}

int setting_username(int fd, char* name, struct node** head){
    struct data data;
    data.descriptor = fd;
    data.name = name;
    if(node_exists(head, &data, compare_name)){
        send(fd, "ATSIUSKVARDA\n", 13, 0);
        return 0;
    }
    struct node* node = get_node(head, &data, compare_descriptors);
    struct data* kazkas = ((struct data*) node->data);
    kazkas->name = (char*)malloc(strlen(name) +1);
    strcpy(((struct data*)node->data)->name, name);
    send(fd, "VARDASOK\n", 9, 0);
    return 1;
}

void send_message(int fd, struct node** head, const char* message, int* fdmax, fd_set* set, int original_message_length, int socket_fd){
    struct data data;
    data.descriptor = fd;
    struct node* node = get_node(head, &data, compare_descriptors);
    char *tmp = strdup(message);
    char begin[1024] = "PRANESIMAS";
    char* name = ((struct data*)node->data)->name;
    int nlen = 0;
    nlen = strlen(name);
     printf("TEST %s\n", begin);
    strcat(begin, name);
     printf("TEST %s\n", begin);
     char test[3] = ": ";
    strcat(begin, test);

    printf("ilgis %ld\n", strlen(begin));
    printf("TEST %s\n", begin);
    strcat(begin, tmp);
    printf("ilgis %ld\n", strlen(begin));
    printf("%s\n", begin);
    int ilg = strlen(begin);
    // printf("%s %d\n", );
    begin[strlen(begin)] = '\n';
    int sent_num;
    for(int j = 0; j <= *fdmax; j++){
        if(FD_ISSET(j, set)) {
            if(j != socket_fd) {
                if((sent_num = send(j, begin, ilg+1, 0)) == -1 || sent_num == 0){
                    fprintf(stderr, "failed to send a msg\n");
                    remove_socket(fd, fdmax,set, head);
                }
            }
        }
    }
}

int main() {
    fd_set master;
    fd_set read_fds;
    int fdmax;
    int user_count = 0;

    
    struct node* head = create_linked_list();

    int socket_fd, client_fd;
    struct addrinfo hints, *server_info, *address_info;
    struct sockaddr_storage client_address;
    socklen_t client_address_size;
    int yes=1;

    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;


    printf("Trying to start server\n");
    if (getaddrinfo(NULL, PORT, &hints, &server_info) != 0) {
        return 1;
    }

    for(address_info = server_info; address_info != NULL; address_info = address_info->ai_next) {
        if ((socket_fd = socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol)) == -1) {
            continue;
        }

        if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
            exit(1);
        }

        if (bind(socket_fd, address_info->ai_addr, address_info->ai_addrlen) == -1) {
            close(socket_fd);
            continue;
        }

        break;
    }

    freeaddrinfo(server_info);

    if (address_info == NULL) {
        fprintf(stderr, "Unable to open a socket\n");
        exit(1);
    }

    printf("Socket created and opened successfully\n");
    if (listen(socket_fd, BACKLOG) == -1) {
        close(socket_fd);
        exit(1);
    }

    FD_SET(socket_fd, &master);

    fdmax = socket_fd;
    char message[1024];
    
    while (1) {
        read_fds = master;
        if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            exit(1);
        }   
      
        for(int i = 0; i <= fdmax; i++) {
            if(!FD_ISSET(i, &read_fds)) continue;

            if(i == socket_fd) {
                
                client_address_size = sizeof client_address;
                client_fd = accept(socket_fd, (struct sockaddr *)&client_address, &client_address_size);
                if (client_fd == -1) {
                    fprintf(stderr, "Failed to accept connection");
                } else {
                    add_connection_to_selector(client_fd, &fdmax, &master, &head);    
                }
            } else {
                int received_length = recv(i, message, 1000, 0);
                
                if (received_length == 0) {
                    fprintf(stderr, "Connection has been closed\n");
                    remove_socket(i, &fdmax, &master, &head);
                }else if (received_length == -1) {
                    fprintf(stderr, "Failed to receive a message\n");
                    remove_socket(i, &fdmax, &master, &head);
                }else{
                    if(message[received_length-1] == '\n') received_length--;
                    if(message[received_length-1] == '\r') received_length--;
                    message[received_length] = '\0';
                    printf("%s\n", message);
                    struct data data;
                    data.descriptor = i;
                    struct node* node = get_node(&head, &data, compare_descriptors);
                  
                    if(((struct data*)(node->data))->name == NULL){
                        setting_username(i, message, &head);
                    }else{
                        send_message(i, &head, message, &fdmax, &master, received_length+1, socket_fd);

                    // message[received_length] = '\0';
                    // switch (string_to_command_number(message)){
                    // case 1:
                    //     if(!setting_username(i, message+8, &head)) continue;
                    //     break;
                    // case 2:

                    //     break;
                    // default:
                    //     break;
                    // }
                    }
                }
            }
        }
    }
    close(socket_fd);

    return 0;
}
