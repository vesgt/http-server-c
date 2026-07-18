#include "http_responses.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

int gm_utc_date(char *date, size_t size) {
    time_t now = time(NULL);
    struct tm utc_time;

    gmtime_r(&now, &utc_time);
    strftime(
        date,
        size,
        "%a, %d %b %Y %H:%M:%S GMT",
        &utc_time
    );

    return 0;
}

void send_all(int fd, const char *buffer) {
    size_t tot_sent = 0;
    size_t buffer_len = strlen(buffer);
    while(tot_sent < buffer_len) {
        ssize_t sent_bytes = send(fd, buffer + tot_sent, buffer_len - tot_sent, 0);
        tot_sent += sent_bytes;
    }
}

void whttp(char **buf, size_t size, const char *write) {
    if (buf == NULL || *buf == NULL)
            return;

    int written = snprintf(*buf, size, "HTTP/1.1 %s", write);

    if (written < 0 || (size_t)written >= size)
        *buf = NULL;
}

void sctorl(int status_code, char *buf, size_t size) {
    switch(status_code) {
        case 100:
            whttp(&buf, size, "100 Continue");
            break;
        case 101:
            whttp(&buf, size, "101 Switching Protocols");
            break;
        case 200:
            whttp(&buf, size, "200 OK");
            break;
        case 201:
            whttp(&buf, size, "201 Created");
            break;
        case 202:
            whttp(&buf, size, "202 Accepted");
            break;
        case 203:
            whttp(&buf, size, "203 Non-Authoritative Information");
            break;
        case 204:
            whttp(&buf, size, "204 No Content");
            break;
        case 205:
            whttp(&buf, size, "205 Reset Content");
            break;
        case 206:
            whttp(&buf, size, "206 Partial Content");
            break;
        case 300:
            whttp(&buf, size, "300 Multiple Choices");
            break;
        case 301:
            whttp(&buf, size, "301 Moved Permanently");
            break;
        case 302:
            whttp(&buf, size, "302 Found");
            break;
        case 303:
            whttp(&buf, size, "303 See Other");
            break;
        case 304:
            whttp(&buf, size, "304 Not Modified");
            break;
        case 305:
            whttp(&buf, size, "305 Use Proxy");
            break;
        case 307:
            whttp(&buf, size, "307 Temporary Redirect");
            break;
        case 308:
            whttp(&buf, size, "308 Permanent Redirect");
            break;
        case 400:
            whttp(&buf, size, "400 Bad Request");
            break;
        case 401:
            whttp(&buf, size, "401 Unauthorized");
            break;
        case 402:
            whttp(&buf, size, "402 Payment Required");
            break;
        case 403:
            whttp(&buf, size, "403 Forbidden");
            break;
        case 404:
            whttp(&buf, size, "404 Not Found");
            break;
        case 405:
            whttp(&buf, size, "405 Method Not Allowed");
            break;
        case 406:
            whttp(&buf, size, "406 Not Acceptable");
            break;
        case 407:
            whttp(&buf, size, "407 Proxy Authentication Timeout");
            break;
        case 408:
            whttp(&buf, size, "408 Request Timeout");
            break;
        case 409:
            whttp(&buf, size, "409 Conflict");
            break;
        case 410:
            whttp(&buf, size, "410 Gone");
            break;
        case 411:
            whttp(&buf, size, "411 Length Required");
            break;
        case 412:
            whttp(&buf, size, "412 Precondition Failed");
            break;
        case 413:
            whttp(&buf, size, "413 Content Too Large");
            break;
        case 414:
            whttp(&buf, size, "414 URI Too Long");
            break;
        case 415:
            whttp(&buf, size, "415 Unsupported Media Type");
            break;
        case 416:
            whttp(&buf, size, "416 Range Not Satisfiable");
            break;
        case 417:
            whttp(&buf, size, "417 Expectation Failed");
            break;
        case 421:
            whttp(&buf, size, "421 Misdirected Request");
            break;
        case 422:
            whttp(&buf, size, "422 Unproccessable Content");
            break;
        case 426:
            whttp(&buf, size, "426 Upgrade Required");
            break;
        case 500:
            whttp(&buf, size, "500 Internal Server Error");
            break;
        case 501:
            whttp(&buf, size, "501 Not Implemented");
            break;
        case 502:
            whttp(&buf, size, "502 Bad Gateway");
            break;
        case 503:
            whttp(&buf, size, "503 Service Unavailable");
            break;
        case 504:
            whttp(&buf, size, "504 Gateway Timeout");
            break;
        case 505:
            whttp(&buf, size, "505 HTTP Version Not Supported");
            break;
    }

    if(buf == NULL)
        perror("sctorl");
}

void any_response(int client_fd, int status_code, char *message) {
    char date[64];
    gm_utc_date(date, sizeof(date));

    char response_line[46];
    char *response_ptr = response_line;
    sctorl(status_code, response_ptr, sizeof(response_line));
    char *formatted_headers;
    int res = asprintf(&formatted_headers,
        "%s\r\n"
        "Server: VesC\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %zu\r\n"
        "Date: %s\r\n"
        "\r\n",
        response_ptr,
        strlen(message),
        (char *)&date);

    if(res < 0) {
        close(client_fd);
        return;
    }

    send_all(client_fd, formatted_headers);
    if(message != NULL)
        send_all(client_fd, message);

    close(client_fd);
}

void NotFound(int client_fd, char *content) {
    any_response(client_fd, 404, content);
}

void OK(int client_fd, char *content) {
    any_response(client_fd, 200, content);
}

void InternalServerError(int client_fd, char *content) {
    any_response(client_fd, 500, content);
}
