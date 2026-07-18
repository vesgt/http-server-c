#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/socket.h>
#include "println.h"
#include "http_req.h"
#include "routing.h"
#include "http_responses.h"

int create_socket(void) {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

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

void handle_client(int client_fd, struct route_table *routing_table) {
    char buffer[4096];
    ssize_t recieved_bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if(recieved_bytes <= 0) {
        close(client_fd);
        return;
    }

    buffer[recieved_bytes] = '\0';
    struct http_request request = {0};
    init_http_request(&request, buffer);
    parse_http_request(&request);

    route_request(&request, routing_table, client_fd);
}

int register_route(struct route_table *route_table, char *method, char *path, route_handler handler_fn) {
    if(route_table->capacity == route_table->count) { // if the remaining extra space is as big as count
        // allocate 5 routes of memory.
        void *ptr = realloc(route_table->routes, sizeof(struct route) * (route_table->capacity + 5));
        if(ptr == NULL) {
            perror("route mem allocation");
            println("Allocating memory for new routes failed.");
            return -1;
        }

        route_table->routes = ptr;

        route_table->capacity += 5;
    }

    route_table->routes[route_table->count].method = method;
    route_table->routes[route_table->count].path = path;
    route_table->routes[route_table->count].handler = handler_fn;

    route_table->count++;

    return 0;
}

void handle_get_root(struct http_request *request, int client_fd, struct params route_params, struct params query_params) {
    println("Handling GET / request");
    // Implement the logic for handling GET / requests here
    char *test_query_param = params_get(&query_params, "test");
    OK(client_fd, "NOICE!!!!");
}

void handle_get_user(struct http_request *request, int client_fd, struct params route_params, struct params query_params) {
    println("Handling GET /users/{id} request");
    // Implement the logic for handling GET /users/{id} requests here
    char *user_id_ptr = params_get(&route_params, "id");

    NotFound(client_fd, user_id_ptr);
}

void handle_post_user(struct http_request *request, int client_fd, struct params route_params, struct params query_params) {
    OK(client_fd, "Created a new user!");
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

    println("Before waiting on connections, build routing table");
    struct route_table route_table = {0};
    route_table.count = 0;
    route_table.capacity = 0;

    register_route(&route_table, "GET", "/", handle_get_root);
    register_route(&route_table, "GET", "/users/{id}", handle_get_user);
    register_route(&route_table, "POST", "/users", handle_post_user);

    println("Waiting for connections...");
    socklen_t addr_size = sizeof(addr);
    while (1) {
        clock_t begin = clock();
        int client_fd = accept(socket_fd, (struct sockaddr *)&addr, &addr_size);
        if(client_fd == -1) {
            perror("accept");
            continue;
        }
        clock_t handle_begin = clock();
        handle_client(client_fd, &route_table);
        clock_t end = clock();
        double handle_client_time_spent = (double)(end - handle_begin) / CLOCKS_PER_SEC;
        double total_time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        println("handle_client executed in %fms", handle_client_time_spent * 100);
        println("total time from acceptance %fms", total_time_spent * 100);
    }

    return 0;
}
