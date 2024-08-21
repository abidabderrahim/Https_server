#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#define PORT 4433
#define CERT_FILE "server.crt"
#define KEY_FILE "server.key"
#define WEB_ROOT "/web_app" // Update this to your web application directory

// Function to initialize OpenSSL library
void init_openssl() {
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ERR_load_BIO_strings();
    ERR_load_crypto_strings();
}

// Function to create and configure SSL context
SSL_CTX* create_context() {
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = TLS_server_method(); // Use TLS server method
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    // Load the server certificate
    if (SSL_CTX_use_certificate_file(ctx, CERT_FILE, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    // Load the server private key
    if (SSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

// Function to handle client connections and serve files
void handle_client(SSL *ssl) {
    char buffer[1024];
    int bytes;
    char *file_path;
    char response[4096];
    int file_fd;
    struct stat file_stat;
    ssize_t bytes_read;

    // Read the HTTP request from the client
    bytes = SSL_read(ssl, buffer, sizeof(buffer) - 1);
    buffer[bytes] = 0;

    // Extract the file path from the HTTP request
    file_path = strtok(buffer, " ");
    file_path = strtok(NULL, " ");
    if (file_path[0] == '/') {
        file_path++; // Remove leading '/'
    }

    if (strlen(file_path) == 0) {
        file_path = "index.html"; // Default file
    }

    // Construct full file path
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s/%s", WEB_ROOT, file_path);

    // Open the file
    if (stat(full_path, &file_stat) == 0 && (file_stat.st_mode & S_IFREG)) {
        file_fd = open(full_path, O_RDONLY);
        if (file_fd >= 0) {
            // Send HTTP header
            snprintf(response, sizeof(response),
                     "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %ld\r\n\r\n",
                     file_stat.st_size);
            SSL_write(ssl, response, strlen(response));

            // Send file content
            while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
                SSL_write(ssl, buffer, bytes_read);
            }

            close(file_fd);
        } else {
            snprintf(response, sizeof(response),
                     "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n"
                     "<html><body><h1>404 Not Found</h1></body></html>");
            SSL_write(ssl, response, strlen(response));
        }
    } else {
        snprintf(response, sizeof(response),
                 "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n"
                 "<html><body><h1>404 Not Found</h1></body></html>");
        SSL_write(ssl, response, strlen(response));
    }
}

// Main function to set up the server and handle incoming connections
int main() {
    int sockfd;
    struct sockaddr_in addr;
    SSL_CTX *ctx;
    SSL *ssl;
    int client;

    // Initialize OpenSSL
    init_openssl();
    ctx = create_context();

    // Create a socket and bind it to the port
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Unable to bind socket");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(sockfd, 1) < 0) {
        perror("Unable to listen on socket");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Accept a new connection
        client = accept(sockfd, NULL, NULL);
        if (client < 0) {
            perror("Unable to accept connection");
            continue;
        }

        // Create a new SSL structure for the connection
        ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client);

        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
        } else {
            handle_client(ssl);
        }

        // Clean up
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(client);
    }

    // Clean up and exit
    close(sockfd);
    SSL_CTX_free(ctx);
    return 0;
}


// init_openssl(): Initializes the OpenSSL library.
// create_context(): Creates and configures the SSL context.
// socket(AF_INET, SOCK_STREAM, 0): Creates a TCP socket.
// memset(&addr, 0, sizeof(addr)): Initializes the addr structure.
// addr.sin_family = AF_INET: Specifies the address family (IPv4).
// addr.sin_addr.s_addr = INADDR_ANY: Accepts connections on any network interface.
// addr.sin_port = htons(PORT): Sets the port number.
// bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)): Binds the socket to the specified port.
// listen(sockfd, 1): Listens for incoming connections. The second argument specifies the maximum number of pending connections.
// accept(sockfd, NULL, NULL): Accepts a new connection.
// SSL_new(ctx): Creates a new SSL structure for the connection.
// SSL_set_fd(ssl, client): Associates the SSL structure with the client socket.
// SSL_accept(ssl): Performs the SSL handshake.
// SSL_shutdown(ssl): Shuts down the SSL connection.
// SSL_free(ssl): Frees the SSL structure.
// close(client): Closes the client socket.
// close(sockfd): Closes the server socket.
// SSL_CTX_free(ctx): Frees the SSL contex
