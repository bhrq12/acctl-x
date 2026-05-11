/*
 * ============================================================================
 *
 *       Filename:  security_log.h
 *
 *    Description:  Security event logging system
 *
 *        Version:  1.0
 *        Created:  2026-04-29
 *       Compiler:  gcc
 *
 * ============================================================================
 */
#ifndef __SECURITY_LOG_H__
#define __SECURITY_LOG_H__

#include <stdint.h>
#include <time.h>

/* Security event severity levels */
typedef enum {
    SEC_LOG_EMERG = 0,   /* System is unusable */
    SEC_LOG_ALERT = 1,   /* Action must be taken immediately */
    SEC_LOG_CRIT = 2,    /* Critical conditions */
    SEC_LOG_ERR = 3,     /* Error conditions */
    SEC_LOG_WARNING = 4, /* Warning conditions */
    SEC_LOG_NOTICE = 5,  /* Normal but significant condition */
    SEC_LOG_INFO = 6,    /* Informational */
    SEC_LOG_DEBUG = 7    /* Debug-level messages */
} sec_log_level_t;

/* Security event types */
typedef enum {
    SEC_EVENT_AUTH_SUCCESS = 1,    /* Authentication successful */
    SEC_EVENT_AUTH_FAILURE = 2,    /* Authentication failed */
    SEC_EVENT_AUTH_TIMEOUT = 3,    /* Authentication timeout */
    SEC_EVENT_RATE_LIMIT = 4,      /* Rate limit exceeded */
    SEC_EVENT_REPLAY_ATTACK = 5,   /* Replay attack detected */
    SEC_EVENT_INVALID_CMD = 6,     /* Invalid command blocked */
    SEC_EVENT_CONFIG_CHANGE = 7,   /* Configuration changed */
    SEC_EVENT_FIRMWARE_UPGRADE = 8,/* Firmware upgrade */
    SEC_EVENT_AP_REGISTER = 9,     /* AP registered */
    SEC_EVENT_AP_DEREGISTER = 10,  /* AP deregistered */
    SEC_EVENT_AP_CONNECT = 11,     /* AP connected */
    SEC_EVENT_AP_DISCONNECT = 12,  /* AP disconnected */
    SEC_EVENT_DTLS_ERROR = 13,     /* DTLS error */
    SEC_EVENT_SEC_VIOLATION = 14,  /* Security violation */
    SEC_EVENT_SYSTEM_ERROR = 15    /* System error */
} sec_event_type_t;

/* Security event structure */
typedef struct sec_log_event_t {
    time_t timestamp;           /* Event timestamp */
    sec_log_level_t level;      /* Severity level */
    sec_event_type_t type;      /* Event type */
    char source_ip[46];         /* Source IP address */
    char mac_address[18];       /* MAC address (if applicable) */
    char username[32];          /* Username (if applicable) */
    char message[512];          /* Event message */
    char details[1024];         /* Additional details */
} sec_log_event_t;

/*
 * sec_log_init - Initialize security logging system
 *
 * Parameters:
 *   log_dir - Directory for log files (default: /var/log/acctl/)
 *   max_files - Maximum number of log files to keep
 *   max_size_mb - Maximum size per log file (MB)
 *
 * Returns:
 *   0 on success, -1 on error
 */
int sec_log_init(const char *log_dir, int max_files, int max_size_mb);

/*
 * sec_log_event - Log a security event
 *
 * Parameters:
 *   level - Severity level
 *   type - Event type
 *   source_ip - Source IP address (can be NULL)
 *   mac - MAC address (can be NULL)
 *   username - Username (can be NULL)
 *   message - Event message
 *   details - Additional details (can be NULL)
 *
 * Returns:
 *   0 on success, -1 on error
 */
int sec_log_event(sec_log_level_t level, sec_event_type_t type,
                 const char *source_ip, const char *mac,
                 const char *username, const char *message,
                 const char *details);

/*
 * sec_log_auth_failure - Log authentication failure
 */
int sec_log_auth_failure(const char *source_ip, const char *username,
                         const char *reason);

/*
 * sec_log_auth_success - Log successful authentication
 */
int sec_log_auth_success(const char *source_ip, const char *username);

/*
 * sec_log_rate_limit - Log rate limit violation
 */
int sec_log_rate_limit(const char *source_ip, const char *mac, int type);

/*
 * sec_log_replay_attack - Log replay attack detection
 */
int sec_log_replay_attack(const char *source_ip, uint32_t random);

/*
 * sec_log_ap_connect - Log AP connection
 */
int sec_log_ap_connect(const char *mac, const char *ip);

/*
 * sec_log_ap_disconnect - Log AP disconnection
 */
int sec_log_ap_disconnect(const char *mac, const char *reason);

/*
 * sec_log_config_change - Log configuration change
 */
int sec_log_config_change(const char *username, const char *change);

/*
 * sec_log_firmware_upgrade - Log firmware upgrade
 */
int sec_log_firmware_upgrade(const char *ap_mac, const char *version);

/*
 * sec_log_get_events - Retrieve security events
 *
 * Parameters:
 *   events - Array to store events
 *   max_events - Maximum number of events to retrieve
 *   start_time - Start time (0 for all)
 *   end_time - End time (0 for all)
 *   level_filter - Only retrieve events >= this level (0 for all)
 *
 * Returns:
 *   Number of events retrieved, -1 on error
 */
int sec_log_get_events(sec_log_event_t *events, int max_events,
                       time_t start_time, time_t end_time,
                       sec_log_level_t level_filter);

/*
 * sec_log_cleanup - Clean up old log files
 */
void sec_log_cleanup(void);

#endif /* __SECURITY_LOG_H__ */