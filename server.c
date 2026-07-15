#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/socket.h>
#include <stdarg.h>

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
        perror("Create socket failed.");
        return -1;
    }

    println("Socket created");
    return socket_fd;
}

void create_socket_addr(struct sockaddr_in *socket_addr, uint32_t address, uint16_t port) {
    socket_addr->sin_family = AF_INET;
    socket_addr->sin_port = htons(port);
    socket_addr->sin_addr.s_addr = address;
}

int bind_socket(int socket_fd, struct sockaddr_in *socket_addr) {
    int bind_result = bind(socket_fd, (struct sockaddr *)socket_addr, sizeof(*socket_addr));
    if(bind_result == -1) {
        perror("Binding socket failed.");
        return -1;
    }

    char addr_string[INET_ADDRSTRLEN];

    if(inet_ntop(AF_INET, &socket_addr->sin_addr, addr_string, sizeof(addr_string)) == NULL) {
        perror("inet_ntop");
        return -1;
    }

    println("Bound socket: addr: %s, port: %d, fd: %d", addr_string, ntohs(socket_addr->sin_port), socket_fd);
    return 0;
}

int main(void) {
    int socket_fd = create_socket();
    if(socket_fd < 0)
        return 1;

    struct sockaddr_in addr = {0};
    create_socket_addr(&addr, INADDR_ANY, 8080);
    int bind_result = bind_socket(socket_fd, &addr);
    if (bind_result == -1) {
        println("Closing socket due to bind failure.");
        close(socket_fd);
        return 1;
    }

    if(listen(socket_fd, 10) == -1) {
        perror("listen failed");
        close(socket_fd);
        return 1;
    }

    println("Waiting for connections...");
    socklen_t addr_size = sizeof(addr);
    while (1) {
        accept(socket_fd, (struct sockaddr *)&addr, &addr_size);
    }

    return 0;
}
