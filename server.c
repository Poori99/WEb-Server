#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PORT 8080
#define WEBROOT "./webroot"

void send_response(int sockfd, const char *http_version, const char *content_type, const char *body)
{
    char response[4096];
    sprintf(response, "%s 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\nConnection: close\r\n\r\n%s",
            http_version, content_type, strlen(body), body);
    write(sockfd, response, strlen(response));
}

void send_file(int sockfd, const char *filename)
{
    char response[4096];
    char buffer[4096];
    char filepath[4096];
    ssize_t bytes_read;
    int fd;
    struct stat filestat;
    sprintf(filepath, "%s/%s", WEBROOT, filename);
    fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        sprintf(response, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
        write(sockfd, response, strlen(response));
        return;
    }
    fstat(fd, &filestat);
    sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %ld\r\nConnection: close\r\n\r\n", filestat.st_size);
    write(sockfd, response, strlen(response));
    while ((bytes_read = read(fd, buffer, 4096)) > 0) {
        write(sockfd, buffer, bytes_read);
    }
    close(fd);
}

int main()
{
    int sockfd, new_sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(struct sockaddr);
    char buffer[4096];
    ssize_t bytes_read;
    const char *http_response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n";
    const char *not_found_response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
    const char *not_implemented_response = "HTTP/1.1 501 Not Implemented\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, addr_len);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, addr_len) < 0) {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 10) < 0) {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        new_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_len);
        if (new_sockfd < 0) {
            perror("accept() failed");
            exit(EXIT_FAILURE);
        }
        
         memset(buffer, 0, 4096);
        bytes_read = read(new_sockfd, buffer, 4095);
        if (bytes_read < 0) {
            perror("read() failed");
            exit(EXIT_FAILURE);
        }

        char method[5], path[1024], http_version[9];
        sscanf(buffer, "%s %s %s", method, path, http_version);

        if (strcmp(method, "GET") != 0) {
            write(new_sockfd, not_implemented_response, strlen(not_implemented_response));
        } else {
            if (strcmp(path, "/") == 0) {
                send_file(new_sockfd, "index.html");
            } else {
                send_file(new_sockfd, path + 1);
            }
        }

        close(new_sockfd);
    }

    close(sockfd);

    return 0;
}









