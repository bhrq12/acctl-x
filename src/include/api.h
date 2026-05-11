/*
 * ============================================================================
 *
 *       Filename:  api.h
 *
 *    Description:  API handler header file for AC Controller
 *
 *        Version:  2.0
 *        Created:  2026-04-26
 *       Revision:  complete implementation for frontend-backend integration
 *       Compiler:  gcc
 *
 *         Author:  jianxi sun (jianxi), ycsunjane@gmail.com
 *   Organization:  OpenWrt AC Controller Project
 *
 * ============================================================================
 */

#ifndef API_H
#define API_H

#include "http.h"
#include <time.h>

#define API_TOKEN_EXPIRY_SEC 3600
#define API_MAX_TOKENS 16
#define API_TOKEN_LEN 64

typedef struct {
    char token[API_TOKEN_LEN];
    time_t created;
    time_t expires;
    char username[64];
} api_token_t;

int api_init(void);
int api_register_route(const char *path, enum http_method method, api_handler_t handler);
int api_authenticate(http_request_t *req);
char *api_get_param(const char *query, const char *name);
char *api_get_path_param(const char *path, int index);

int api_generate_token(const char *username, char *token_out, size_t token_len);
int api_validate_token(const char *token);
int api_invalidate_token(const char *token);
void api_cleanup_expired_tokens(void);

int api_handler_status(http_request_t *req, http_response_t *resp);
int api_handler_aps(http_request_t *req, http_response_t *resp);
int api_handler_ap_detail(http_request_t *req, http_response_t *resp);
int api_handler_ap_reboot(http_request_t *req, http_response_t *resp);
int api_handler_ap_config(http_request_t *req, http_response_t *resp);
int api_handler_ap_upgrade(http_request_t *req, http_response_t *resp);
int api_handler_config_get(http_request_t *req, http_response_t *resp);
int api_handler_config_put(http_request_t *req, http_response_t *resp);
int api_handler_alarms(http_request_t *req, http_response_t *resp);
int api_handler_alarm_ack(http_request_t *req, http_response_t *resp);
int api_handler_alarms_ack_all(http_request_t *req, http_response_t *resp);
int api_handler_firmware_list(http_request_t *req, http_response_t *resp);
int api_handler_firmware_upload(http_request_t *req, http_response_t *resp);
int api_handler_firmware_delete(http_request_t *req, http_response_t *resp);
int api_handler_auth_login(http_request_t *req, http_response_t *resp);
int api_handler_auth_refresh(http_request_t *req, http_response_t *resp);
int api_handler_auth_logout(http_request_t *req, http_response_t *resp);

#endif /* API_H */