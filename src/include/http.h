/*
 * ============================================================================
 *
 *       Filename:  http.h
 *
 *    Description:  HTTP server header file for AC Controller
 *
 *        Version:  1.0
 *        Created:  2026-04-26
 *       Revision:  initial implementation
 *       Compiler:  gcc
 *
 *         Author:  jianxi sun (jianxi), ycsunjane@gmail.com
 *   Organization:  OpenWrt AC Controller Project
 *
 * ============================================================================
 */

#ifndef HTTP_H
#define HTTP_H

#include <stdint.h>

/* HTTP server configuration */
#define HTTP_PORT 8080
#define HTTP_MAX_CONNECTIONS 10
#define HTTP_MAX_REQUEST_SIZE 16384
#define HTTP_TIMEOUT 30

/* HTTP methods */
enum http_method {
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_UNKNOWN
};

/* HTTP request structure */
typedef struct {
    enum http_method method;
    char path[256];
    char query[1024];
    char body[HTTP_MAX_REQUEST_SIZE];
    size_t body_len;
    char headers[1024];
} http_request_t;

/* HTTP response structure */
typedef struct {
    int status_code;
    char status_message[64];
    char headers[1024];
    char body[HTTP_MAX_REQUEST_SIZE];
    size_t body_len;
} http_response_t;

/* API handler function type */
typedef int (*api_handler_t)(http_request_t *req, http_response_t *resp);

/* API route structure */
typedef struct {
    const char *path;
    enum http_method method;
    api_handler_t handler;
} api_route_t;

/* HTTP server functions */
int http_server_init(void);
int http_server_start(void);
void http_server_stop(void);

/* API functions */
int api_init(void);
int api_register_route(const char *path, enum http_method method, api_handler_t handler);

/* HTTP utility functions */
enum http_method http_parse_method(const char *method);
void http_parse_request(const char *data, size_t len, http_request_t *req);
void http_build_response(http_response_t *resp, char *buffer, size_t *len);
void http_response_json(http_response_t *resp, int status_code, const char *json);
void http_response_error(http_response_t *resp, int status_code, const char *message);
char *http_request_get_header(http_request_t *req, const char *name);

#endif /* HTTP_H */
