#include "http_req.h"
#include "println.h"
#include <ctype.h>
#include <string.h>

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
    println("Recieved request: method: %s, target: %s, version: %s", method, target, version);

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
