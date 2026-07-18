#ifndef ROUTING_INCLUDED
#define ROUTING_INCLUDED
#include <sys/types.h>
#include "http_req.h"
#define MAX_ROUTES 4096

struct span {
    char *ptr;
    size_t len;
};

struct param {
    struct span key;
    char *value;
};

struct params {
    struct param params[8];
    int count;
};

typedef void (*route_handler)(struct http_request *request, int client_fd, struct params route_params, struct params query_params);

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

void init_route_params(struct params *params, char *request_path, char *route_path);
void route_request(struct http_request *request, struct route_table *route_table, int client_fd);
char *params_get(struct params *params, const char *key);

#endif
