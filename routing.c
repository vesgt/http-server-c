#include "routing.h"
#include "http_req.h"
#include <string.h>

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

        if(*rt_path_ptr == '\0' && *rq_path_ptr == '\0') {
            struct route_params params = {0};
            init_route_params(&params, request->path, route_table->routes[i].path);
            route_table->routes[i].handler(request, client_fd, params);
        }
    }
}
