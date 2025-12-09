// Example C program using socket library
// NOTE: This requires a userland C runtime to be set up
// For now, this serves as documentation of the API

#include "../include/syscalls.h"
#include "../include/socket.h"

void server_example(void) {
    print("Starting server...\n");
    
    // Create and listen on server socket
    int server_fd = sock_create_server("/sock/myserver", SOCK_TYPE_STREAM, 5);
    if (server_fd < 0) {
        print("Failed to create server\n");
        exit(1);
    }
    
    print("Server listening on /sock/myserver\n");
    
    // Accept a connection
    int client = accept(server_fd);
    if (client < 0) {
        print("Failed to accept connection\n");
        close(server_fd);
        exit(1);
    }
    
    print("Client connected!\n");
    
    // Receive message
    char buffer[256];
    int received = recv(client, buffer, sizeof(buffer) - 1, 0);
    if (received > 0) {
        buffer[received] = '\0';
        print("Received: ");
        print(buffer);
        print("\n");
    }
    
    // Send response
    const char* response = "Hello from server!";
    send(client, response, 18, 0);
    
    // Cleanup
    close(client);
    sock_destroy(server_fd, "/sock/myserver");
    
    print("Server shutting down\n");
}

void client_example(void) {
    print("Starting client...\n");
    
    // Connect to server
    int sockfd = sock_connect_client("/sock/myserver", SOCK_TYPE_STREAM);
    if (sockfd < 0) {
        print("Failed to connect to server\n");
        exit(1);
    }
    
    print("Connected to server!\n");
    
    // Send message
    const char* message = "Hello from client!";
    if (sock_send_all(sockfd, message, 18) < 0) {
        print("Failed to send message\n");
        close(sockfd);
        exit(1);
    }
    
    print("Message sent\n");
    
    // Receive response
    char buffer[256];
    int received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (received > 0) {
        buffer[received] = '\0';
        print("Received: ");
        print(buffer);
        print("\n");
    }
    
    // Cleanup
    close(sockfd);
    
    print("Client shutting down\n");
}

// In a real implementation, you'd fork() here to run server and client concurrently
// For this example, they're just separate functions
void _start(void) {
    // This would be run by parent process
    server_example();
    
    // This would be run by child process after fork()
    // client_example();
    
    exit(0);
}
