#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // POSIX constants and types .
#include <sys/types.h> // definitions for data types used in system calls .
#include <sys/stat.h> // definitions for file status and information .
#include <dirent.h> // directory operations .
#include <fcntl.h> // definitions for file control options .
#include <arpa/inet.h> // definitions for internet operations .
#include <openssl/ssl.h> // Openssl library header for SSL/TLS .
#include <openssl/err.h> // Openssl header for error handling .

#define PORT 4433
#define CERT_FILEL "server.crt" // Filename of the SSL certificate .
#define KEY_FILE "server.key" // Filename of the SSL private key .
#define WEB_ROOT "/home/rootkill/Desktop/web_app"

void init_openssl(){
	SSL_library_init(); // initializes the SSL library .
	OpenSSL_add_all_algorithms(); // Loads all available cryptographic algorithms .
	SSL_load_error_strings(); // Loads human readable error strings for SSL errors .
}

// Create and Configure SSL Context .

SSL_CTX* create_contect(){
	const SSL_METHOD *method;
	SSL_CTX *ctx;

	method = TLS_server_method(); // method for TLS/SSL server operations .
	ctx = SSL_CTX_new(method); // for create SSL context .
	if(!ctx){
		perror("Unable To Create SSL Context");
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}
	if(SSL_CTX_use_certificate_file(ctx, CERT_FILE, SSL_FILETYPE_PEM) <= 0){
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}
	if(SSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, SSL_FILETYPE_PEM) <= 0){
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}
	return ctx;
}

// handle client connections .

void handle_client(SSL *ssl){
	char buffer[1024];
	int bytes;
	char *file_path;
	char response[4096];
	int file_fd;
	struct stat file_stat;
	size_t bytes_read;

	bytes = SSL_read(ssl, buffer, sizeof(buffer) - 1);
	buffer[bytes] = 0;

	file_path = strtok(buffer, " ");
	file_path = strtok(NULL, " ");
	if(file_path[0] == '/'){
		file_path++;
	}
	if(strlen(file_apth) == 0){
		file_path = "index.html";
	}

	char full_path[1024];
	snprintf(full_path, sizeof(full_path), "%s/%s", WEB_ROOT, file_path);
	
	if(stat(full_path, &file_stat) == 0 && (file_stat.st_mode & S_IFREG)){
		file_fd = open(full_path, ORDONLY);
		if(file_fd >= 0){
			snprintf(response, sizeof(response),"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %ld\r\n\r\n", file_stat.st_size);
			SSL_write(ssl, response, strlen(response));
			while((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0){
				SSLL_write(ssl, buffer, bytes_read);
			}
			close(filel_fd);
		}else{
			snprintf(response, sizeof(response), "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n"
                     "<html><body><h1>404 Not Found</h1></body></html>");
			SSL_write(ssl, response, strlen(response));
		}
	}else{
		snprintf(response, sizeof(response), "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n"
                 "<html><body><h1>404 Not Found</h1></body></html>");
		SSL_write(ssl, response, strlen(response));
	}
}

int main(){
	int sockfd;
	struct sockaddr_in addr;
	SSL_CTX *ctx;
	SSL *ssl;
	int client;

	init_openssl();
	ctx = create_context();

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(PORT);

	if(bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0){
		perror("Unable to Listen On Socket");
		exit(EXIT_FAILURE);
	}

	while(1){
		client = accept(sockfd, NULL, NULL);
		if(client < 0){
			perror("Unable TO Accept Connection");
			continue;
		}

		ssl = SSL_new(ctx);
		SLL_set_fd(ssl, client);

		if(SSL_accept(ssl) <= 0){
			ERR_print_error_fp(stderr);
		}else{
			handle_client(ssl);
		}

		SSL_shutdown(ssl);
		SSL_free(ssl);
		close(client);
	}

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
