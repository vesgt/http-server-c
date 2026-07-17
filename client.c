#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

void println(const char *format, ...) {
    va_list args;

    va_start(args, format);
    vprintf(format, args);
    printf("\n");

    va_end(args);
}

int create_socket(void) {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd == -1) {
        perror("create socket failed.");
        return -1;
    }

    return socket_fd;
}

void init_socket_addr(struct sockaddr_in *socket_addr, in_addr_t address, uint16_t port) {
    socket_addr->sin_family = AF_INET;
    socket_addr->sin_port = htons(port);
    socket_addr->sin_addr.s_addr = address;
}

int main(void) {
    println("creating socket");
    int socket_fd = create_socket();
    if (socket_fd < 0) {
        return 1;
    }
    println("socket created");

    struct sockaddr_in addr = {0};
    init_socket_addr(&addr, inet_addr("127.0.0.1"), 8080);
    int connect_result = connect(socket_fd, (struct sockaddr*)&addr, sizeof(addr));
    if (connect_result == -1) {
        perror("connect");
        return 1;
    }

    println("connected!");
    println("trying to send a request");

    const char *request =
        "GET /users/42 HTTP/1.1\r\n"
        "HOST: localhost:8080\r\n"
        "\r\n";

    int send_res = send(socket_fd, request, strlen(request), 0);
    if (send_res < 0)
        return 1;

    println("sent successfully");
    println("trying to recieve");

    char buffer[4096];
    ssize_t bytes = recv(socket_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes < 0)
        return 1;

    println("recieved successfully!");
    buffer[bytes] = '\0';
    println("%s", buffer);

    return 0;
}
