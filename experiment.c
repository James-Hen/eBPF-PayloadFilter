#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>

#define A_PORT 1145
#define B_PORT 5141

void host_A() {
    /*
        Host A acts like a server, who accepts connection
    */
    int server_fd;
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address;
    int addrlen = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(A_PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address,
                                sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    int new_socket;
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                    (socklen_t*)&addrlen))<0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    char buffer[1024] = {0};
    char *hello = "Hello!", *areuok = "Are you OK?";
    // A sends "Hello!"
    send(new_socket, hello, strlen(hello), 0);
    // After ACK, A sends "Are you OK?"
    send(new_socket, areuok, strlen(areuok), 0);
    // B sends "Hello!"
    int valread = read(new_socket, buffer, 1024);
}

void host_B() {
    /*
        Host B acts like a client
    */
    int sock;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }
   
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(B_PORT);
       
    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
   
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    char buffer[1024] = {0};
    // B reads "Hello!" and ACK
    int valread = read(sock, buffer, 1024);
    // B sends "Hello!"
    char *hello = "Hello!";
    send(sock, hello ,strlen(hello) ,0);
    // B reads "Are you OK?" and ACK
    valread = read(sock, buffer, 1024);
}

int main(int argc, char const *argv[]) {
    int B_pid = fork();
    if (B_pid == 0) { // B
        host_B();
    }
    else { // A
        host_A();
    }
    return 0;
}
