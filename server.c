#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/socket.h>
#include <stdarg.h>
#include <ctype.h>

#define MAX_HEADER_AMOUNT 124
#define MAX_HEADERS_TOTAL_SIZE 8192
#define MAX_HEADER_SIZE 4096

#define MAX_ROUTES 4096

struct http_header {
  char *name;
  char *value;
};

struct http_request {
  char *method;
  char *path;
  char *version;

  struct http_header headers[MAX_HEADER_AMOUNT];
  size_t header_len;
  int header_count;

  char *body;
  size_t body_len;
};

struct span {
    char *ptr;
    size_t len;
};

struct route_param {
    struct span key;
    char *value;
};

struct route_params {
    struct route_param route_params[8];
    int count;
};

typedef void (*route_handler)(struct http_request *request, int client_fd, struct route_params route_params);

struct route {
    char *method;
    char *path;
    route_handler handler;
};

struct route_table {
    struct route *routes;
    int capacity;
    int count;
};

void println(const char *format, ...) {
    va_list args;

    va_start(args, format);
    vprintf(format, args);
    printf("\n");

    va_end(args);
}

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

void init_http_request(struct http_request *request, char *buffer) {
    char *separator = strstr(buffer, "\r\n\r\n");
    char *body = separator + 4; // Skip the delimiter strstr uses.
    *separator = '\0'; // Turn the remaining delimiter in the headers + request line to an ending. Making buffer only that.
    // this block of memory originally: bla bla bla \r\n\r\n bla bla bla \0, becomes bla bla bla \0 bla bla bla \0.
    // reading from the body pointer give the second half and reading from the buffer pointer gives the first.

    char *method = strtok(buffer, " ");
    char *target = strtok(NULL, " ");
    char *version = strchr(target, '\0') + 1;
    char *version_end = strchr(version, '\r');
    *version_end = '\0';
    println("Parsed top level, method: %s, target: %s, version: %s", method, target, version);

    request->method = method;
    request->path = target;
    request->version = version;
    request->header_count = 0;
}

void parse_http_request(struct http_request *request) {
    char *version_end = strchr(request->version, '\0');

    char *ptr = version_end + 2; // end of request line
    while (ptr != NULL) {
        char *line = ptr; // full line

        char *line_end = strstr(line, "\r\n");
        if(line_end != NULL) {
            *line_end = '\0';
            ptr = line_end + 2;
        } else {
            ptr = NULL;
        }

        char *line_separator = strchr(line, ':'); // pointer to colon (would give value with colon if read)...
        *line_separator = '\0'; // line is now header name
        char *value = line_separator + 1; // move value of header after the \0 separator, failure to do this would make it an empty string instead.

        while(isspace((unsigned char)*value)) {
            value++;
        }

        if(request->header_count < MAX_HEADER_AMOUNT) {
            request->headers[request->header_count].name = line;
            request->headers[request->header_count].value = value;

            request->header_count++;
        }
    }
}

char *params_get(struct route_params *params, const char *key) {
    size_t key_len = strlen(key);
    for(int i = 0; i < params->count; i++) {
        if(params->route_params[i].key.len == key_len && strncmp(params->route_params[i].key.ptr, key, params->route_params[i].key.len) == 0) {
            return params->route_params[i].value;
        }
    }

    return NULL;
}

struct span is_dynamic_part(char *ptr) {
    size_t len_to_sep = strcspn(ptr, "/");

    struct span dynamic_part = {0};
    if(len_to_sep >= 2 && ptr[0] == '{' && ptr[len_to_sep - 1] == '}') {
        dynamic_part.ptr = ptr + 1;
        dynamic_part.len = len_to_sep - 2;
        return dynamic_part;
    }

    dynamic_part.ptr = NULL;
    return dynamic_part;
}

void init_route_params(struct route_params *params, char *request_path, char *route_path) {
    while (*request_path != '\0' && *route_path != '\0') {
        int request_len_to_sep = strcspn(request_path, "/");
        int route_len_to_sep = strcspn(route_path, "/");

        char request_sep = request_path[request_len_to_sep];

        struct span dynamic_part = is_dynamic_part(route_path);
        if(dynamic_part.ptr != NULL) {
            params->route_params[params->count].key = dynamic_part;

            *(request_path + request_len_to_sep) = '\0';
            params->route_params[params->count].value = request_path;

            params->count++;
        }

        request_path += request_len_to_sep;
        route_path += route_len_to_sep;

        if(request_sep == '/')
            request_path++;

        if(*route_path == '/')
            route_path++;
    }
}

void route_request(struct http_request *request, struct route_table *route_table, int client_fd) {
    for(int i = 0; i < route_table->count; i++) {
        char *rt_method_ptr = route_table->routes[i].method;
        char *rq_method_ptr = request->method;

        if(strcmp(rt_method_ptr, rq_method_ptr) != 0)
            continue;

        char *rt_path_ptr = route_table->routes[i].path + 1;
        char *rq_path_ptr = request->path + 1; // + 1 on both skips over the inital slash.
        while(*rt_path_ptr != '\0' && *rq_path_ptr != '\0') {
            int rt_len_to_sep = strcspn(rt_path_ptr, "/");
            int rq_len_to_sep = strcspn(rq_path_ptr, "/");

            if(is_dynamic_part(rt_path_ptr).ptr != NULL) {
            } else if(rt_len_to_sep != rq_len_to_sep || strncmp(rq_path_ptr, rt_path_ptr, rt_len_to_sep) != 0) {
                break;
            }

            rt_path_ptr += rt_len_to_sep;
            rq_path_ptr += rq_len_to_sep;

            if(*rt_path_ptr == '/')
                rt_path_ptr++;

            if(*rq_path_ptr == '/')
                rq_path_ptr++;
        }

        if(*rt_path_ptr == '\0' && *rq_path_ptr == '\0') {
            struct route_params params = {0};
            init_route_params(&params, request->path, route_table->routes[i].path);
            route_table->routes[i].handler(request, client_fd, params);
        }
    }
}

void handle_client(int client_fd, struct route_table *routing_table) {
    char buffer[4096];
    ssize_t recieved_bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if(recieved_bytes <= 0) {
        close(client_fd);
        return;
    }

    buffer[recieved_bytes] = '\0';
    println("Recieved request: %s", buffer);

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

void OK(int client_fd, char *content) {

}

void handle_get_root(struct http_request *request, int client_fd, struct route_params route_params) {
    println("Handling GET / request");
    // Implement the logic for handling GET / requests here
    char *response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 12\r\n"
        "\r\n"
        "Hello World!";

    println("Sending response: %s", response);

    int send_res = send(client_fd, response, strlen(response), 0);

    if(send_res < 0) {
        perror("send");
        println("send failed");
    }

    println("send success!");

    close(client_fd);

    println("closed client");
}

void handle_get_user(struct http_request *request, int client_fd, struct route_params route_params) {
    println("Handling GET /users/{id} request");
    // Implement the logic for handling GET /users/{id} requests here
    char *user_id_ptr = params_get(&route_params, "id");
    int user_id = atoi(user_id_ptr);
    println("%d", user_id);
    char *response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "/users/{id}";

    println("Sending response: %s", response);

    int send_res = send(client_fd, response, strlen(response), 0);

    if(send_res < 0) {
        perror("send");
        println("send failed");
    }

    println("send success!");

    close(client_fd);

    println("closed client");
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

    println("Waiting for connections...");
    socklen_t addr_size = sizeof(addr);
    while (1) {
        int client_fd = accept(socket_fd, (struct sockaddr *)&addr, &addr_size);
        if(client_fd == -1) {
            perror("accept");
            continue;
        }
        handle_client(client_fd, &route_table);
    }

    return 0;
}
