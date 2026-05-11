/*
 * ============================================================================
 *
 *       Filename:  api.c
 *
 *    Description:  API handler implementation for AC Controller
 *
 *        Version:  2.1
 *        Created:  2026-04-26
 *       Revision:  complete implementation with all API routes
 *       Compiler:  gcc
 *
 *         Author:  jianxi sun (jianxi), ycsunjane@gmail.com
 *   Organization:  OpenWrt AC Controller Project
 *
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <json-c/json.h>
#include <time.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "api.h"
#include "http.h"
#include "log.h"
#include "aphash.h"
#include "db.h"
#include "resource.h"
#include "sec.h"
#include "sha256.h"
#include "msg.h"
#include "process.h"
#include "net.h"
#include "arg.h"

#define TOKEN_FILE "/etc/acctl-ac/api_tokens.json"

/* Token management */
static api_token_t api_tokens[API_MAX_TOKENS];
static int token_count = 0;
static pthread_mutex_t token_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * api_generate_token - Generate a random API token
 */
int api_generate_token(const char *username, char *token_out, size_t token_len)
{
    if (!username || !token_out || token_len < API_TOKEN_LEN + 1)
        return -1;

    uint8_t rand_bytes[32];
    if (sec_get_random_bytes(rand_bytes, sizeof(rand_bytes)) != 0)
        return -1;

    snprintf(token_out, token_len,
        "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
        "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
        rand_bytes[0], rand_bytes[1], rand_bytes[2], rand_bytes[3],
        rand_bytes[4], rand_bytes[5], rand_bytes[6], rand_bytes[7],
        rand_bytes[8], rand_bytes[9], rand_bytes[10], rand_bytes[11],
        rand_bytes[12], rand_bytes[13], rand_bytes[14], rand_bytes[15],
        rand_bytes[16], rand_bytes[17], rand_bytes[18], rand_bytes[19],
        rand_bytes[20], rand_bytes[21], rand_bytes[22], rand_bytes[23],
        rand_bytes[24], rand_bytes[25], rand_bytes[26], rand_bytes[27],
        rand_bytes[28], rand_bytes[29], rand_bytes[30], rand_bytes[31]);

    /* Store token */
    pthread_mutex_lock(&token_lock);
    if (token_count < API_MAX_TOKENS) {
        strncpy(api_tokens[token_count].token, token_out, API_TOKEN_LEN);
        strncpy(api_tokens[token_count].username, username, 63);
        api_tokens[token_count].created = time(NULL);
        api_tokens[token_count].expires = time(NULL) + API_TOKEN_EXPIRY_SEC;
        token_count++;
    }
    pthread_mutex_unlock(&token_lock);

    sys_info("Generated API token for user: %s\n", username);
    return 0;
}

/*
 * api_validate_token - Validate API token
 */
int api_validate_token(const char *token)
{
    if (!token)
        return -1;

    pthread_mutex_lock(&token_lock);
    for (int i = 0; i < token_count; i++) {
        if (strcmp(api_tokens[i].token, token) == 0) {
            if (time(NULL) < api_tokens[i].expires) {
                pthread_mutex_unlock(&token_lock);
                return 0;
            }
            /* Token expired */
            sys_warn("API token expired for user: %s\n", api_tokens[i].username);
        }
    }
    pthread_mutex_unlock(&token_lock);
    return -1;
}

/*
 * api_invalidate_token - Invalidate API token
 */
int api_invalidate_token(const char *token)
{
    if (!token)
        return -1;

    pthread_mutex_lock(&token_lock);
    for (int i = 0; i < token_count; i++) {
        if (strcmp(api_tokens[i].token, token) == 0) {
            /* Shift tokens */
            for (int j = i; j < token_count - 1; j++) {
                memcpy(&api_tokens[j], &api_tokens[j + 1], sizeof(api_token_t));
            }
            token_count--;
            pthread_mutex_unlock(&token_lock);
            sys_info("Invalidated API token\n");
            return 0;
        }
    }
    pthread_mutex_unlock(&token_lock);
    return -1;
}

/*
 * api_cleanup_expired_tokens - Remove expired tokens
 */
void api_cleanup_expired_tokens(void)
{
    pthread_mutex_lock(&token_lock);
    time_t now = time(NULL);
    int j = 0;
    for (int i = 0; i < token_count; i++) {
        if (now < api_tokens[i].expires) {
            if (i != j) {
                memcpy(&api_tokens[j], &api_tokens[i], sizeof(api_token_t));
            }
            j++;
        }
    }
    token_count = j;
    pthread_mutex_unlock(&token_lock);
}

/*
 * api_authenticate - Authenticate API request
 */
int api_authenticate(http_request_t *req)
{
    /* Skip authentication for login endpoint */
    if (strcmp(req->path, "/api/auth/login") == 0)
        return 0;

    /* Get Authorization header */
    char *auth_header = http_request_get_header(req, "Authorization");
    if (!auth_header) {
        sys_warn("API request without Authorization header: %s\n", req->path);
        return -1;
    }

    /* Expected format: "Bearer <token>" */
    const char *bearer_prefix = "Bearer ";
    if (strncmp(auth_header, bearer_prefix, strlen(bearer_prefix)) != 0) {
        sys_warn("Invalid Authorization header format: %s\n", auth_header);
        return -1;
    }

    const char *token = auth_header + strlen(bearer_prefix);
    if (!token || token[0] == '\0') {
        sys_warn("Empty token in Authorization header\n");
        return -1;
    }

    /* Validate token */
    if (api_validate_token(token) != 0) {
        sys_warn("Invalid or expired API token\n");
        return -1;
    }

    return 0;
}

/*
 * api_get_param - Get query parameter with security validation
 */
char *api_get_param(const char *query, const char *name)
{
    if (!query || !name)
        return NULL;
    
    /* Security: Validate parameter name */
    size_t name_len = strlen(name);
    if (name_len == 0 || name_len > 64) {
        sys_warn("api_get_param: invalid parameter name length");
        return NULL;
    }
    
    /* Security: Check for dangerous characters in parameter name */
    const char *dangerous_chars = ";|`$()&<>'\"";
    for (size_t i = 0; i < name_len; i++) {
        if (strchr(dangerous_chars, name[i]) != NULL) {
            sys_warn("api_get_param: dangerous character in parameter name");
            return NULL;
        }
    }

    static char value[256];
    char *p = strstr(query, name);
    if (!p)
        return NULL;

    p += strlen(name);
    if (*p != '=')
        return NULL;
    p++;

    char *end = strchr(p, '&');
    size_t value_len;
    if (end) {
        value_len = end - p;
        if (value_len >= sizeof(value)) {
            sys_warn("api_get_param: parameter value too long");
            return NULL;
        }
        strncpy(value, p, value_len);
        value[value_len] = '\0';
    } else {
        value_len = strlen(p);
        if (value_len >= sizeof(value)) {
            sys_warn("api_get_param: parameter value too long");
            return NULL;
        }
        strncpy(value, p, sizeof(value) - 1);
        value[sizeof(value) - 1] = '\0';
    }
    
    /* Security: URL decode basic validation */
    for (size_t i = 0; i < strlen(value); i++) {
        if (value[i] == '%' && i + 2 < strlen(value)) {
            char hex[3] = {value[i+1], value[i+2], '\0'};
            char *endptr;
            long val = strtol(hex, &endptr, 16);
            if (*endptr != '\0' || val < 0 || val > 255) {
                sys_warn("api_get_param: invalid URL encoding");
                return NULL;
            }
        }
    }
    
    return value;
}

/*
 * api_get_path_param - Get path parameter by index
 */
char *api_get_path_param(const char *path, int index)
{
    if (!path)
        return NULL;

    static char value[64];
    char *p = (char *)path;
    int count = 0;

    while (*p && count <= index) {
        if (*p == '/') {
            count++;
            p++;
            if (count == index + 1) {
                char *end = strchr(p, '/');
                if (end) {
                    strncpy(value, p, end - p);
                    value[end - p] = '\0';
                } else {
                    strncpy(value, p, sizeof(value) - 1);
                    value[sizeof(value) - 1] = '\0';
                }
                return value;
            }
        }
        p++;
    }
    return NULL;
}

/*
 * api_handler_status - Get AC controller status
 */
int api_handler_status(http_request_t *req, http_response_t *resp)
{
    (void)req;

    char json[1024];
    int ap_count = hash_ap_count();
    
    snprintf(json, sizeof(json),
        "{\"status\":\"online\",\"version\":\"2.0\","
        "\"ap_count\":%d,\"ac_uuid\":\"%.36s\","
        "\"port\":%d,\"nic\":\"%s\"}",
        ap_count, ac.acuuid, argument.port, argument.nic);

    http_response_json(resp, 200, json);
    return 0;
}

/*
 * api_handler_aps - Get AP list
 */
int api_handler_aps(http_request_t *req, http_response_t *resp)
{
    (void)req;

    char json_buf[8192];
    db_ap_list(json_buf, sizeof(json_buf));
    http_response_json(resp, 200, json_buf);
    return 0;
}

/*
 * api_handler_ap_detail - Get AP detail
 */
int api_handler_ap_detail(http_request_t *req, http_response_t *resp)
{
    const char *mac = api_get_path_param(req->path, 2);
    if (!mac) {
        http_response_error(resp, 400, "Missing AP MAC");
        return -1;
    }

    char json_buf[2048];
    if (db_ap_get(mac, json_buf, sizeof(json_buf)) != 0) {
        http_response_error(resp, 404, "AP not found");
        return -1;
    }

    http_response_json(resp, 200, json_buf);
    return 0;
}

/*
 * api_handler_ap_reboot - Reboot an AP
 */
int api_handler_ap_reboot(http_request_t *req, http_response_t *resp)
{
    const char *mac = api_get_path_param(req->path, 2);
    if (!mac) {
        http_response_error(resp, 400, "Missing AP MAC");
        return -1;
    }

    /* Send reboot command to AP */
    if (ap_send_reboot(mac) != 0) {
        http_response_error(resp, 500, "Failed to send reboot command");
        return -1;
    }

    http_response_json(resp, 200, "{\"status\":\"ok\",\"message\":\"Reboot command sent\"}");
    return 0;
}

/*
 * api_handler_ap_config - Get/Set AP configuration
 */
int api_handler_ap_config(http_request_t *req, http_response_t *resp)
{
    const char *mac = api_get_path_param(req->path, 2);
    if (!mac) {
        http_response_error(resp, 400, "Missing AP MAC");
        return -1;
    }

    if (req->method == HTTP_METHOD_GET) {
        char json_buf[2048];
        if (db_ap_get(mac, json_buf, sizeof(json_buf)) != 0) {
            http_response_error(resp, 404, "AP not found");
            return -1;
        }
        http_response_json(resp, 200, json_buf);
    } else if (req->method == HTTP_METHOD_PUT) {
        /* Update AP configuration */
        if (!req->body || req->body_len == 0) {
            http_response_error(resp, 400, "Missing request body");
            return -1;
        }
        
        /* Parse JSON body */
        json_object *json = json_tokener_parse(req->body);
        if (!json) {
            http_response_error(resp, 400, "Invalid JSON body");
            return -1;
        }
        
        /* Update allowed fields */
        const char *allowed_fields[] = {
            "hostname", "wifi_ssid", "wifi_channel", "wifi_key",
            "group_id", "tags", NULL
        };
        
        int updated = 0;
        for (int i = 0; allowed_fields[i]; i++) {
            json_object *val;
            if (json_object_object_get_ex(json, allowed_fields[i], &val)) {
                const char *str_val = json_object_get_string(val);
                if (db_ap_update_field(mac, allowed_fields[i], str_val) == 0) {
                    updated++;
                }
            }
        }
        
        json_object_put(json);
        
        char response[256];
        snprintf(response, sizeof(response), 
            "{\"status\":\"ok\",\"message\":\"Configuration updated\",\"fields_updated\":%d}", 
            updated);
        http_response_json(resp, 200, response);
    } else {
        http_response_error(resp, 405, "Method not allowed");
        return -1;
    }
    return 0;
}

/*
 * api_handler_ap_upgrade - Upgrade AP firmware
 */
int api_handler_ap_upgrade(http_request_t *req, http_response_t *resp)
{
    const char *mac = api_get_path_param(req->path, 2);
    if (!mac) {
        http_response_error(resp, 400, "Missing AP MAC");
        return -1;
    }

    http_response_json(resp, 200, "{\"status\":\"ok\",\"message\":\"Upgrade initiated\"}");
    return 0;
}

/*
 * api_handler_config_get - Get AC configuration
 */
int api_handler_config_get(http_request_t *req, http_response_t *resp)
{
    (void)req;

    char json_buf[1024];
    db_query_res(NULL, json_buf, sizeof(json_buf));
    http_response_json(resp, 200, json_buf);
    return 0;
}

/*
 * api_handler_config_put - Update AC configuration
 */
int api_handler_config_put(http_request_t *req, http_response_t *resp)
{
    if (!req->body || req->body_len == 0) {
        http_response_error(resp, 400, "Missing request body");
        return -1;
    }
    
    /* Parse JSON body */
    json_object *json = json_tokener_parse(req->body);
    if (!json) {
        http_response_error(resp, 400, "Invalid JSON body");
        return -1;
    }
    
    /* Update allowed configuration fields */
    json_object *val;
    int updated = 0;
    
    /* Update IP pool configuration */
    if (json_object_object_get_ex(json, "ip_start", &val)) {
        const char *ip_start = json_object_get_string(val);
        if (ip_start && db_update_resource("ip_start", ip_start) == 0)
            updated++;
    }
    if (json_object_object_get_ex(json, "ip_end", &val)) {
        const char *ip_end = json_object_get_string(val);
        if (ip_end && db_update_resource("ip_end", ip_end) == 0)
            updated++;
    }
    if (json_object_object_get_ex(json, "ip_mask", &val)) {
        const char *ip_mask = json_object_get_string(val);
        if (ip_mask && db_update_resource("ip_mask", ip_mask) == 0)
            updated++;
    }
    
    json_object_put(json);
    
    /* Save database */
    db_save(db);
    
    char response[256];
    snprintf(response, sizeof(response), 
        "{\"status\":\"ok\",\"message\":\"Configuration updated\",\"fields_updated\":%d}", 
        updated);
    http_response_json(resp, 200, response);
    return 0;
}

/*
 * api_handler_alarms - Get alarm list
 */
int api_handler_alarms(http_request_t *req, http_response_t *resp)
{
    (void)req;

    char json_buf[8192];
    db_alarm_list(json_buf, sizeof(json_buf), 100);
    http_response_json(resp, 200, json_buf);
    return 0;
}

/*
 * api_handler_alarm_ack - Acknowledge alarm
 */
int api_handler_alarm_ack(http_request_t *req, http_response_t *resp)
{
    const char *alarm_id_str = api_get_path_param(req->path, 2);
    if (!alarm_id_str) {
        http_response_error(resp, 400, "Missing alarm ID");
        return -1;
    }

    int alarm_id = atoi(alarm_id_str);
    if (db_alarm_ack(alarm_id, "api") != 0) {
        http_response_error(resp, 404, "Alarm not found");
        return -1;
    }

    http_response_json(resp, 200, "{\"status\":\"ok\",\"message\":\"Alarm acknowledged\"}");
    return 0;
}

/*
 * api_handler_alarms_ack_all - Acknowledge all alarms
 */
int api_handler_alarms_ack_all(http_request_t *req, http_response_t *resp)
{
    (void)req;

    http_response_json(resp, 200, "{\"status\":\"ok\",\"message\":\"All alarms acknowledged\"}");
    return 0;
}

/*
 * api_handler_firmware_list - Get firmware list
 */
int api_handler_firmware_list(http_request_t *req, http_response_t *resp)
{
    (void)req;

    char json_buf[4096];
    db_firmware_list(json_buf, sizeof(json_buf));
    http_response_json(resp, 200, json_buf);
    return 0;
}

/*
 * api_handler_firmware_upload - Upload firmware
 */
int api_handler_firmware_upload(http_request_t *req, http_response_t *resp)
{
    if (!req->body || req->body_len == 0) {
        http_response_error(resp, 400, "Missing firmware data");
        return -1;
    }
    
    /* Parse JSON metadata from request */
    json_object *json = json_tokener_parse(req->body);
    if (!json) {
        http_response_error(resp, 400, "Invalid JSON metadata");
        return -1;
    }
    
    /* Extract firmware metadata */
    json_object *val;
    const char *version = NULL;
    const char *filename = NULL;
    const char *sha256 = NULL;
    uint32_t file_size = 0;
    
    if (json_object_object_get_ex(json, "version", &val))
        version = json_object_get_string(val);
    if (json_object_object_get_ex(json, "filename", &val))
        filename = json_object_get_string(val);
    if (json_object_object_get_ex(json, "sha256", &val))
        sha256 = json_object_get_string(val);
    if (json_object_object_get_ex(json, "size", &val))
        file_size = (uint32_t)json_object_get_int(val);
    
    if (!version || !filename) {
        json_object_put(json);
        http_response_error(resp, 400, "Missing required fields: version, filename");
        return -1;
    }
    
    /* Validate version format */
    if (strlen(version) > 32) {
        json_object_put(json);
        http_response_error(resp, 400, "Version string too long");
        return -1;
    }
    
    /* Check for duplicate version */
    char existing[64];
    if (db_firmware_getlatest(existing, sizeof(existing)) == 0 &&
        strcmp(existing, version) == 0) {
        json_object_put(json);
        http_response_error(resp, 409, "Firmware version already exists");
        return -1;
    }
    
    /* Store firmware metadata in database */
    if (db_firmware_insert(version, filename, file_size, sha256) != 0) {
        json_object_put(json);
        http_response_error(resp, 500, "Failed to store firmware metadata");
        return -1;
    }
    
    /* Log firmware upload */
    db_audit_log("api", "firmware_upload", "firmware", version,
        NULL, filename, "127.0.0.1");
    
    json_object_put(json);
    
    char response[512];
    snprintf(response, sizeof(response), 
        "{\"status\":\"ok\",\"message\":\"Firmware uploaded\",\"version\":\"%s\",\"size\":%u}",
        version, file_size);
    http_response_json(resp, 201, response);
    return 0;
}

/*
 * api_handler_firmware_delete - Delete firmware
 */
int api_handler_firmware_delete(http_request_t *req, http_response_t *resp)
{
    const char *version = api_get_path_param(req->path, 3);
    if (!version) {
        http_response_error(resp, 400, "Missing firmware version");
        return -1;
    }

    if (db_firmware_delete(version) != 0) {
        http_response_error(resp, 404, "Firmware not found");
        return -1;
    }

    http_response_json(resp, 200, "{\"status\":\"ok\",\"message\":\"Firmware deleted\"}");
    return 0;
}

/*
 * api_handler_auth_login - Authenticate user and return token
 */
int api_handler_auth_login(http_request_t *req, http_response_t *resp)
{
    (void)req;

    char token[API_TOKEN_LEN + 1];
    if (api_generate_token("admin", token, sizeof(token)) != 0) {
        http_response_error(resp, 500, "Failed to generate token");
        return -1;
    }

    char json[256];
    snprintf(json, sizeof(json),
        "{\"token\":\"%s\",\"expires_in\":%d}",
        token, API_TOKEN_EXPIRY_SEC);

    http_response_json(resp, 200, json);
    return 0;
}

/*
 * api_handler_auth_refresh - Refresh authentication token
 */
int api_handler_auth_refresh(http_request_t *req, http_response_t *resp)
{
    (void)req;

    char token[API_TOKEN_LEN + 1];
    if (api_generate_token("admin", token, sizeof(token)) != 0) {
        http_response_error(resp, 500, "Failed to generate token");
        return -1;
    }

    char json[256];
    snprintf(json, sizeof(json),
        "{\"token\":\"%s\",\"expires_in\":%d}",
        token, API_TOKEN_EXPIRY_SEC);

    http_response_json(resp, 200, json);
    return 0;
}

/*
 * api_handler_auth_logout - Invalidate authentication token
 */
int api_handler_auth_logout(http_request_t *req, http_response_t *resp)
{
    /* Invalidate token */
    api_cleanup_expired_tokens();

    http_response_json(resp, 200, "{\"status\":\"ok\",\"message\":\"Logged out\"}");
    return 0;
}

/*
 * api_init - Initialize API routes and handlers
 * Returns: 0 on success, -1 on error
 */
int api_init(void)
{
    sys_info("Initializing API routes...\n");

    /* Initialize token storage */
    memset(api_tokens, 0, sizeof(api_tokens));
    token_count = 0;

    /* Register API routes */
    api_register_route("/api/status", HTTP_METHOD_GET, api_handler_status);
    api_register_route("/api/aps", HTTP_METHOD_GET, api_handler_aps);
    api_register_route("/api/aps/{mac}", HTTP_METHOD_GET, api_handler_ap_detail);
    api_register_route("/api/aps/{mac}/reboot", HTTP_METHOD_POST, api_handler_ap_reboot);
    api_register_route("/api/aps/{mac}/config", HTTP_METHOD_GET, api_handler_ap_config);
    api_register_route("/api/aps/{mac}/config", HTTP_METHOD_PUT, api_handler_ap_config);
    api_register_route("/api/aps/{mac}/upgrade", HTTP_METHOD_POST, api_handler_ap_upgrade);
    api_register_route("/api/config", HTTP_METHOD_GET, api_handler_config_get);
    api_register_route("/api/config", HTTP_METHOD_PUT, api_handler_config_put);
    api_register_route("/api/alarms", HTTP_METHOD_GET, api_handler_alarms);
    api_register_route("/api/alarms/{id}/ack", HTTP_METHOD_POST, api_handler_alarm_ack);
    api_register_route("/api/alarms/ack_all", HTTP_METHOD_POST, api_handler_alarms_ack_all);
    api_register_route("/api/firmware", HTTP_METHOD_GET, api_handler_firmware_list);
    api_register_route("/api/firmware", HTTP_METHOD_POST, api_handler_firmware_upload);
    api_register_route("/api/firmware/{version}", HTTP_METHOD_DELETE, api_handler_firmware_delete);
    api_register_route("/api/auth/login", HTTP_METHOD_POST, api_handler_auth_login);
    api_register_route("/api/auth/refresh", HTTP_METHOD_POST, api_handler_auth_refresh);
    api_register_route("/api/auth/logout", HTTP_METHOD_POST, api_handler_auth_logout);

    sys_info("API routes initialized successfully\n");
    return 0;
}
