#ifndef HTTP_RESPONSES_INCLUDED
#define HTTP_RESPONSES_INCLUDED
#include <sys/types.h>

void sctorl(int status_code, char *buf, size_t size);
void any_response(int client_fd, int status_code, char *message);
void OK(int client_fd, char *success_message);
void NotFound(int client_fd, char *error_message);
void InternalServerError(int client_fd, char *error_message);

#endif
