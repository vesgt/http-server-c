#ifndef ROUTING_INCLUDED
#define ROUTING_INCLUDED
#include <sys/types.h>
#include "http_req.h"
#define MAX_ROUTES 4096

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

void init_route_params(struct route_params *params, char *request_path, char *route_path);
void route_request(struct http_request *request, struct route_table *route_table, int client_fd);

#endif
