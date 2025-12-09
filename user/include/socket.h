#ifndef USER_SOCKET_H
#define USER_SOCKET_H

#include "syscalls.h"

/**
 * Socket library - high-level wrappers for Unix domain sockets
 * 
 * Usage example:
 * 
 * Server:
 *   int server_fd = sock_create_server("/sock/myserver", SOCK_TYPE_STREAM, 5);
 *   while (1) {
 *       int client = accept(server_fd);
 *       char buf[256];
 *       int n = recv(client, buf, sizeof(buf), 0);
 *       send(client, "Response", 8, 0);
 *       close(client);
 *   }
 * 
 * Client:
 *   int client_fd = sock_connect_client("/sock/myserver", SOCK_TYPE_STREAM);
 *   send(client_fd, "Hello", 5, 0);
 *   char buf[256];
 *   int n = recv(client_fd, buf, sizeof(buf), 0);
 *   close(client_fd);
 */

/**
 * Create a server socket and start listening
 * @param path Socket path in sockfs (e.g., "/sock/myserver")
 * @param type Socket type (SOCK_TYPE_STREAM, SOCK_TYPE_DGRAM, SOCK_TYPE_RAW)
 * @param backlog Maximum pending connections
 * @return Socket fd on success, -1 on error
 */
static inline int sock_create_server(const char* path, int type, int backlog) {
    // Create socket
    int sockfd = socket(type);
    if (sockfd < 0) {
        return -1;
    }
    
    // Note: In sockfs, the socket is automatically bound to its unique name
    // We don't need a separate bind() syscall
    
    // Start listening
    if (listen(sockfd, backlog) < 0) {
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

/**
 * Connect to a server socket
 * @param path Socket path in sockfs (e.g., "/sock/myserver")
 * @param type Socket type (should match server)
 * @return Socket fd on success, -1 on error
 */
static inline int sock_connect_client(const char* path, int type) {
    // Create socket
    int sockfd = socket(type);
    if (sockfd < 0) {
        return -1;
    }
    
    // Connect to server
    if (connect(sockfd, path) < 0) {
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

/**
 * Send all data (handles partial sends)
 * @param sockfd Socket file descriptor
 * @param buf Data buffer
 * @param len Length of data
 * @return Number of bytes sent, or -1 on error
 */
static inline int sock_send_all(int sockfd, const void* buf, size_t len) {
    size_t total_sent = 0;
    const char* ptr = (const char*)buf;
    
    while (total_sent < len) {
        int sent = send(sockfd, ptr + total_sent, len - total_sent, 0);
        if (sent < 0) {
            return -1;
        }
        total_sent += sent;
    }
    
    return total_sent;
}

/**
 * Receive all data (blocks until len bytes received)
 * @param sockfd Socket file descriptor
 * @param buf Data buffer
 * @param len Length of data to receive
 * @return Number of bytes received, or -1 on error
 */
static inline int sock_recv_all(int sockfd, void* buf, size_t len) {
    size_t total_received = 0;
    char* ptr = (char*)buf;
    
    while (total_received < len) {
        int received = recv(sockfd, ptr + total_received, len - total_received, 0);
        if (received < 0) {
            return -1;
        }
        if (received == 0) {
            // Connection closed
            break;
        }
        total_received += received;
    }
    
    return total_received;
}

/**
 * Close socket and unlink from filesystem
 * @param sockfd Socket file descriptor
 * @param path Socket path (for unlinking)
 */
static inline void sock_destroy(int sockfd, const char* path) {
    close(sockfd);
    if (path) {
        unlink(path);
    }
}

#endif // USER_SOCKET_H
