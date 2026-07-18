#ifndef HTTP_REQUEST_INCLUDED
#define HTTP_REQUEST_INCLUDED
#include <sys/types.h>
#define MAX_HEADER_AMOUNT 124
#define MAX_HEADERS_TOTAL_SIZE 8192
#define MAX_HEADER_SIZE 4096

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

void init_http_request(struct http_request *request, char *buffer);
void parse_http_request(struct http_request *request);

#endif
