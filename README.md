# HTTPS Server in C

This project is a simple HTTPS server implemented in C using OpenSSL. The server is capable of serving static files such as HTML, CSS, and JavaScript with TLS encryption, ensuring secure communication between the client and server.

## Features

- **TLS/SSL Encryption**: Utilizes OpenSSL to provide secure communication over HTTPS.
- **Static File Serving**: Serves static web files from a specified directory.
- **Self-Signed Certificates**: Allows the use of self-signed certificates for development purposes.

## Prerequisites

Before running the server, make sure you have the following installed:

- **OpenSSL**: Required for SSL/TLS functionality.
- **GCC**: A C compiler for building the server.
- **A Web Browser**: For testing the HTTPS server (e.g., Chrome, Firefox).

## Setup

1. **Generate SSL Certificates**:

   To use HTTPS, you need a certificate and a private key. You can generate a self-signed certificate with the following command:

   ```bash
   openssl req -new -x509 -days 365 -nodes -out server.crt -keyout server.key
   gcc -o Https_Server Https_Server.c -lssl -lcrypto
   ./Https_Server
   https://localhost:4433
