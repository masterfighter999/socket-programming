#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

#define SERVER_IP "127.0.0.1"  // Server IP address
#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // Create UDP socket
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket == -1) {
        perror("Error creating client socket");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(PORT);

    // Loop to continuously send messages
    while (1) {
        printf("Enter message to send (or 'quit' to exit): ");
        fgets(buffer, BUFFER_SIZE, stdin);

        // Remove newline character from fgets
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[--len] = '\0';
        }

        // Check if user wants to quit
        if (strcmp(buffer, "quit") == 0) {
            break;
        }

        // Sending data to the server
        if (sendto(client_socket, buffer, strlen(buffer), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            perror("Error sending data to server");
            continue;  // Continue to next iteration of loop
        }

        printf("Data sent to server\n");
    }

    close(client_socket);
    return 0;
}

