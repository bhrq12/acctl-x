/*
 * ============================================================================
 *
 *       Filename:  http.c
 *
 *    Description:  HTTP server implementation for AC Controller
 *
 *        Version:  1.0
 *        Created:  2026-04-26
 *       Revision:  initial implementation
 *       Compiler:  gcc
 *
 *         Author:  jianxi sun (jianxi), ycsunjane@gmail.com
 *   Organization:  OpenWrt AC Controller Project
 *
 * SPDX-License-Identifier: Apache-2.0
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <microhttpd.h>

#include "http.h"
#include "api.h"
#include "log.h"

static struct MHD_Daemon *http_daemon = NULL;
static api_route_t *routes = NULL;
static int route_count = 0;
static int route_capacity = 10;

char *http_request_get_header(http_request_t *req, const char *name);

static char *http_headers_buffer = NULL;
static size_t http_headers_buffer_size = 0;

char *http_request_get_header(http_request_t *req, const char *name)
{
    if (!req || !name) {
        return NULL;
    }

    if (http_headers_buffer && http_headers_buffer_size > 0) {
        char header_line[512];
        snprintf(header_line, sizeof(header_line), "%s:", name);

        char *found = strstr(http_headers_buffer, header_line);
        if (found) {
            found += strlen(header_line);
            while (*found == ' ') found++;

            char *end = strchr(found, '\r');
            if (!end) end = strchr(found, '\n');
            if (!end) end = found + strlen(found);

            size_t value_len = end - found;
            if (value_len > 0 && value_len < 256) {
                static char header_value[256];
                strncpy(header_value, found, value_len);
                header_value[value_len] = '\0';
                return header_value;
            }
        }
    }

    return NULL;
}

int http_extract_headers(struct MHD_Connection *connection, char *buffer, size_t buffer_size)
{
    if (!connection || !buffer || buffer_size == 0) {
        return -1;
    }

    size_t pos = 0;
    int header_count = 0;

    memset(buffer, 0, buffer_size);

    /* List of commonly used HTTP headers to extract */
    const char *common_headers[] = {
        "Content-Type", "Content-Length", "Authorization",
        "Accept", "Host", "User-Agent",
        "Connection", "Cache-Control", "Origin",
        "Referer", "X-Requested-With", NULL
    };

    for (int i = 0; common_headers[i] != NULL && pos < buffer_size - 1; i++) {
        const char *hvalue = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, common_headers[i]);
        if (hvalue) {
            size_t len = snprintf(buffer + pos, buffer_size - pos, "%s: %s\r\n", common_headers[i], hvalue);
            pos += len;
            header_count++;
        }
    }

    return header_count;
}

/* Forward declarations */
int http_handle_request(http_request_t *req, http_response_t *resp);

/*
 * validate_request_path - Comprehensive path validation to prevent traversal
 * Returns: 1 if valid, 0 if invalid
 */
static int validate_request_path(const char *path)
{
    if (!path || strlen(path) > 256)
        return 0;

    static const char *dangerous_patterns[] = {
        "..", "./", "/.", "~",
        "%2e", "%2E", "%2e%2e", "%2E%2E",
        "%252e", "%252E",
        NULL
    };

    for (int i = 0; dangerous_patterns[i]; i++) {
        if (strcasestr(path, dangerous_patterns[i])) {
            sys_warn("Dangerous path pattern detected: %s", dangerous_patterns[i]);
            return 0;
        }
    }

    if (memchr(path, '\0', strlen(path)))
        return 0;

    return 1;
}

/* HTTP request handler */
static enum MHD_Result http_request_handler(void *cls, struct MHD_Connection *connection,
                               const char *url, const char *method,
                               const char *version, const char *upload_data,
                               size_t *upload_data_size, void **con_cls)
{
    static int dummy;
    static char headers_buffer[4096];
    http_request_t req;
    http_response_t resp;
    char buffer[HTTP_MAX_REQUEST_SIZE * 2];
    size_t buffer_len;
    int ret;

    if (*con_cls == NULL) {
        *con_cls = &dummy;
        return MHD_YES;
    }

    if (*upload_data_size != 0) {
        return MHD_YES;
    }

    /* Validate input parameters */
    if (!connection || !method) {
        sys_warn("HTTP request with invalid connection or method");
        return MHD_NO;
    }

    /* Log request details */
    sys_debug("HTTP request: %s %s %s", method, url ? url : "", version ? version : "");

    /* Initialize request and response */
    memset(&req, 0, sizeof(req));
    memset(&resp, 0, sizeof(resp));
    memset(headers_buffer, 0, sizeof(headers_buffer));

    http_extract_headers(connection, headers_buffer, sizeof(headers_buffer));
    strncpy(req.headers, headers_buffer, sizeof(req.headers) - 1);
    req.headers[sizeof(req.headers) - 1] = '\0';

    http_headers_buffer = headers_buffer;
    http_headers_buffer_size = sizeof(headers_buffer);

    /* Parse HTTP method */
    req.method = http_parse_method(method);
    if (req.method == HTTP_METHOD_UNKNOWN) {
        sys_warn("HTTP request with unknown method: %s", method);
        http_response_error(&resp, 405, "Method not allowed");
        http_build_response(&resp, buffer, &buffer_len);
        struct MHD_Response *mhd_resp = MHD_create_response_from_buffer(
            buffer_len, (void *)buffer, MHD_RESPMEM_MUST_COPY);
        if (mhd_resp) {
            MHD_add_response_header(mhd_resp, "Allow", "GET, POST, PUT, DELETE");
            MHD_queue_response(connection, resp.status_code, mhd_resp);
            MHD_destroy_response(mhd_resp);
        }
        return MHD_YES;
    }

    /* Parse URL */
    if (!url || url[0] == '\0') {
        sys_warn("HTTP request with empty URL");
        http_response_error(&resp, 400, "Empty URL");
        http_build_response(&resp, buffer, &buffer_len);
        struct MHD_Response *mhd_resp = MHD_create_response_from_buffer(
            buffer_len, (void *)buffer, MHD_RESPMEM_MUST_COPY);
        if (mhd_resp) {
            MHD_queue_response(connection, resp.status_code, mhd_resp);
            MHD_destroy_response(mhd_resp);
        }
        return MHD_YES;
    }

    char *query_start = strchr(url, '?');
    if (query_start) {
        if ((size_t)(query_start - url) < sizeof(req.path)) {
            size_t path_len = query_start - url;
            strncpy(req.path, url, path_len);
            req.path[path_len] = '\0';
            strncpy(req.query, query_start + 1, sizeof(req.query) - 1);
            req.query[sizeof(req.query) - 1] = '\0';
        } else {
            sys_warn("HTTP request path too long: %s", url);
            http_response_error(&resp, 400, "Request path too long");
            http_build_response(&resp, buffer, &buffer_len);
            struct MHD_Response *mhd_resp = MHD_create_response_from_buffer(
                buffer_len, (void *)buffer, MHD_RESPMEM_MUST_COPY);
            if (mhd_resp) {
                MHD_queue_response(connection, resp.status_code, mhd_resp);
                MHD_destroy_response(mhd_resp);
            }
            return MHD_YES;
        }
    } else {
        strncpy(req.path, url, sizeof(req.path) - 1);
        req.path[sizeof(req.path) - 1] = '\0';
    }

    /* Security: Comprehensive path traversal attack prevention */
    if (!validate_request_path(req.path)) {
        sys_warn("HTTP request with path traversal attempt: %s", req.path);
        http_response_error(&resp, 400, "Invalid path");
        http_build_response(&resp, buffer, &buffer_len);
        struct MHD_Response *mhd_resp = MHD_create_response_from_buffer(
            buffer_len, (void *)buffer, MHD_RESPMEM_MUST_COPY);
        if (mhd_resp) {
            MHD_queue_response(connection, resp.status_code, mhd_resp);
            MHD_destroy_response(mhd_resp);
        }
        return MHD_YES;
    }

    /* Handle the request */
    ret = http_handle_request(&req, &resp);

    /* Log response status */
    sys_debug("HTTP response: %d %s", resp.status_code, resp.status_message);

    /* Build response */
    http_build_response(&resp, buffer, &buffer_len);

    /* Send response */
    struct MHD_Response *mhd_resp = MHD_create_response_from_buffer(
        buffer_len, (void *)buffer, MHD_RESPMEM_MUST_COPY);

    if (!mhd_resp) {
        sys_err("Failed to create MHD response");
        return MHD_NO;
    }

    /* Security headers */
    MHD_add_response_header(mhd_resp, "X-Content-Type-Options", "nosniff");
    MHD_add_response_header(mhd_resp, "X-Frame-Options", "DENY");
    MHD_add_response_header(mhd_resp, "X-XSS-Protection", "1; mode=block");
    MHD_add_response_header(mhd_resp, "Strict-Transport-Security", "max-age=31536000; includeSubDomains");

    int ret_send = MHD_queue_response(connection, resp.status_code, mhd_resp);
    MHD_destroy_response(mhd_resp);

    return (enum MHD_Result)ret_send;
}

/* Handle HTTP request */
int http_handle_request(http_request_t *req, http_response_t *resp)
{
    /* Find matching route */
    for (int i = 0; i < route_count; i++) {
        if (routes[i].method == req->method &&
            strcmp(routes[i].path, req->path) == 0) {
            return routes[i].handler(req, resp);
        }
    }

    /* Route not found */
    http_response_error(resp, 404, "Route not found");
    return -1;
}

/* Initialize HTTP server */
int http_server_init(void)
{
    /* Allocate route table */
    routes = malloc(route_capacity * sizeof(api_route_t));
    if (!routes) {
        sys_err("Failed to allocate route table\n");
        return -1;
    }

    /* Initialize API routes */
    if (api_init() != 0) {
        sys_err("Failed to initialize API routes\n");
        free(routes);
        return -1;
    }

    return 0;
}

/* Start HTTP server */
int http_server_start(void)
{
    http_daemon = MHD_start_daemon(
        MHD_USE_SELECT_INTERNALLY, HTTP_PORT,
        NULL, NULL,
        &http_request_handler, NULL,
        MHD_OPTION_END);

    if (!http_daemon) {
        sys_err("Failed to start HTTP server\n");
        return -1;
    }

    sys_info("HTTP server started on port %d\n", HTTP_PORT);
    return 0;
}

/* Stop HTTP server */
void http_server_stop(void)
{
    if (http_daemon) {
        MHD_stop_daemon(http_daemon);
        http_daemon = NULL;
        sys_info("HTTP server stopped\n");
    }

    if (routes) {
        free(routes);
        routes = NULL;
        route_count = 0;
        route_capacity = 10;
    }
}

/* Register API route */
int api_register_route(const char *path, enum http_method method, api_handler_t handler)
{
    if (route_count >= route_capacity) {
        route_capacity *= 2;
        api_route_t *new_routes = realloc(routes, route_capacity * sizeof(api_route_t));
        if (!new_routes) {
            sys_err("Failed to reallocate route table\n");
            return -1;
        }
        routes = new_routes;
    }

    routes[route_count].path = path;
    routes[route_count].method = method;
    routes[route_count].handler = handler;
    route_count++;

    sys_info("Registered API route: %s %s\n",
             method == HTTP_METHOD_GET ? "GET" :
             method == HTTP_METHOD_POST ? "POST" :
             method == HTTP_METHOD_PUT ? "PUT" :
             method == HTTP_METHOD_DELETE ? "DELETE" : "UNKNOWN",
             path);

    return 0;
}

/* Parse HTTP method */
enum http_method http_parse_method(const char *method)
{
    if (strcmp(method, "GET") == 0) {
        return HTTP_METHOD_GET;
    } else if (strcmp(method, "POST") == 0) {
        return HTTP_METHOD_POST;
    } else if (strcmp(method, "PUT") == 0) {
        return HTTP_METHOD_PUT;
    } else if (strcmp(method, "DELETE") == 0) {
        return HTTP_METHOD_DELETE;
    } else {
        return HTTP_METHOD_UNKNOWN;
    }
}

/* Parse HTTP request */
void http_parse_request(const char *data, size_t len, http_request_t *req)
{
    /* Simple HTTP request parsing */
    /* This is a basic implementation, you may need to enhance it */
}

/* Build HTTP response */
void http_build_response(http_response_t *resp, char *buffer, size_t *len)
{
    int pos = 0;

    /* Status line */
    pos += snprintf(buffer + pos, HTTP_MAX_REQUEST_SIZE * 2 - pos,
                   "HTTP/1.1 %d %s\r\n",
                   resp->status_code, resp->status_message);

    /* Headers */
    if (resp->headers[0]) {
        pos += snprintf(buffer + pos, HTTP_MAX_REQUEST_SIZE * 2 - pos,
                       "%s\r\n", resp->headers);
    }

    /* Content-Length header */
    pos += snprintf(buffer + pos, HTTP_MAX_REQUEST_SIZE * 2 - pos,
                   "Content-Length: %zu\r\n", resp->body_len);

    /* CORS header */
    pos += snprintf(buffer + pos, HTTP_MAX_REQUEST_SIZE * 2 - pos,
                   "Access-Control-Allow-Origin: *\r\n");
    pos += snprintf(buffer + pos, HTTP_MAX_REQUEST_SIZE * 2 - pos,
                   "Access-Control-Allow-Methods: GET, POST, PUT, DELETE\r\n");
    pos += snprintf(buffer + pos, HTTP_MAX_REQUEST_SIZE * 2 - pos,
                   "Access-Control-Allow-Headers: Content-Type, Authorization\r\n");

    /* End of headers */
    pos += snprintf(buffer + pos, HTTP_MAX_REQUEST_SIZE * 2 - pos, "\r\n");

    /* Body */
    if (resp->body_len > 0) {
        memcpy(buffer + pos, resp->body, resp->body_len);
        pos += resp->body_len;
    }

    *len = pos;
}

/* Send JSON response */
void http_response_json(http_response_t *resp, int status_code, const char *json)
{
    resp->status_code = status_code;

    switch (status_code) {
        case 200:
            strncpy(resp->status_message, "OK", sizeof(resp->status_message) - 1);
            break;
        case 201:
            strncpy(resp->status_message, "Created", sizeof(resp->status_message) - 1);
            break;
        case 400:
            strncpy(resp->status_message, "Bad Request", sizeof(resp->status_message) - 1);
            break;
        case 401:
            strncpy(resp->status_message, "Unauthorized", sizeof(resp->status_message) - 1);
            break;
        case 403:
            strncpy(resp->status_message, "Forbidden", sizeof(resp->status_message) - 1);
            break;
        case 404:
            strncpy(resp->status_message, "Not Found", sizeof(resp->status_message) - 1);
            break;
        case 500:
            strncpy(resp->status_message, "Internal Server Error", sizeof(resp->status_message) - 1);
            break;
        default:
            strncpy(resp->status_message, "Unknown Status", sizeof(resp->status_message) - 1);
            break;
    }
    resp->status_message[sizeof(resp->status_message) - 1] = '\0';

    strncpy(resp->headers, "Content-Type: application/json", sizeof(resp->headers) - 1);
    resp->headers[sizeof(resp->headers) - 1] = '\0';

    if (json) {
        strncpy(resp->body, json, sizeof(resp->body) - 1);
        resp->body[sizeof(resp->body) - 1] = '\0';
    } else {
        resp->body[0] = '\0';
    }
    resp->body_len = strlen(resp->body);
}

/* Send error response */
void http_response_error(http_response_t *resp, int status_code, const char *message)
{
    char json[1024];
    snprintf(json, sizeof(json),
             "{\"error\":{\"code\":%d,\"message\":\"%s\"}}",
             status_code, message);
    http_response_json(resp, status_code, json);
}
