/**
 * @file HttpResponse.h
 * @brief Struct for maintaining a cached HTTP response.
 * @author Josh Hellauer
 * @date 2024-11-04
 */

#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#define MAX 5

typedef struct HttpResponse {
    char* filename;
    char* response; // file contents only 
    unsigned long filesize;
    struct timespec access_time;
} HttpResponse;

void free_http_response(HttpResponse* resp);

#endif