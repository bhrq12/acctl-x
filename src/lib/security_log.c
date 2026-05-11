/*
 * ============================================================================
 *
 *       Filename:  security_log.c
 *
 *    Description:  Security event logging system implementation
 *
 *        Version:  1.0
 *        Created:  2026-04-29
 *       Compiler:  gcc
 *
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>

#include "security_log.h"
#include "log.h"

/* Default configuration */
#define DEFAULT_LOG_DIR "/var/log/acctl/"
#define DEFAULT_MAX_FILES 10
#define DEFAULT_MAX_SIZE_MB 10

/* Static variables */
static char g_log_dir[256] = DEFAULT_LOG_DIR;
static int g_max_files = DEFAULT_MAX_FILES;
static int g_max_size_bytes = DEFAULT_MAX_SIZE_MB * 1024 * 1024;
static FILE *g_log_file = NULL;
static pthread_mutex_t g_log_lock = PTHREAD_MUTEX_INITIALIZER;

/* Event type to string mapping */
static const char *event_type_str(sec_event_type_t type) {
    switch (type) {
        case SEC_EVENT_AUTH_SUCCESS: return "AUTH_SUCCESS";
        case SEC_EVENT_AUTH_FAILURE: return "AUTH_FAILURE";
        case SEC_EVENT_AUTH_TIMEOUT: return "AUTH_TIMEOUT";
        case SEC_EVENT_RATE_LIMIT: return "RATE_LIMIT";
        case SEC_EVENT_REPLAY_ATTACK: return "REPLAY_ATTACK";
        case SEC_EVENT_INVALID_CMD: return "INVALID_CMD";
        case SEC_EVENT_CONFIG_CHANGE: return "CONFIG_CHANGE";
        case SEC_EVENT_FIRMWARE_UPGRADE: return "FIRMWARE_UPGRADE";
        case SEC_EVENT_AP_REGISTER: return "AP_REGISTER";
        case SEC_EVENT_AP_DEREGISTER: return "AP_DEREGISTER";
        case SEC_EVENT_AP_CONNECT: return "AP_CONNECT";
        case SEC_EVENT_AP_DISCONNECT: return "AP_DISCONNECT";
        case SEC_EVENT_DTLS_ERROR: return "DTLS_ERROR";
        case SEC_EVENT_SEC_VIOLATION: return "SEC_VIOLATION";
        case SEC_EVENT_SYSTEM_ERROR: return "SYSTEM_ERROR";
        default: return "UNKNOWN";
    }
}

/* Level to string mapping */
static const char *level_str(sec_log_level_t level) {
    switch (level) {
        case SEC_LOG_EMERG: return "EMERG";
        case SEC_LOG_ALERT: return "ALERT";
        case SEC_LOG_CRIT: return "CRIT";
        case SEC_LOG_ERR: return "ERR";
        case SEC_LOG_WARNING: return "WARNING";
        case SEC_LOG_NOTICE: return "NOTICE";
        case SEC_LOG_INFO: return "INFO";
        case SEC_LOG_DEBUG: return "DEBUG";
        default: return "UNKNOWN";
    }
}

/* Get current log file path */
static int get_log_path(char *path, size_t len) {
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    snprintf(path, len, "%ssecurity-%04d%02d%02d.log",
             g_log_dir, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    return 0;
}

/* Rotate log files */
static int rotate_logs(void) {
    char path[256];
    get_log_path(path, sizeof(path));
    
    /* Check current file size */
    struct stat st;
    if (stat(path, &st) == 0 && st.st_size >= g_max_size_bytes) {
        /* Need to rotate */
        char backup_path[256];
        for (int i = g_max_files - 1; i > 0; i--) {
            snprintf(backup_path, sizeof(backup_path), "%s.%d", path, i);
            char prev_path[256];
            snprintf(prev_path, sizeof(prev_path), "%s.%d", path, i - 1);
            
            if (i == 1) {
                rename(path, backup_path);
            } else {
                rename(prev_path, backup_path);
            }
        }
        
        /* Close current file */
        if (g_log_file) {
            fclose(g_log_file);
            g_log_file = NULL;
        }
    }
    
    return 0;
}

/* Open log file */
static FILE *open_log_file(void) {
    if (g_log_file) {
        return g_log_file;
    }
    
    char path[256];
    get_log_path(path, sizeof(path));
    
    g_log_file = fopen(path, "a");
    if (!g_log_file) {
        sys_err("Failed to open security log file: %s\n", strerror(errno));
        return NULL;
    }
    
    /* Enable line buffering */
    setvbuf(g_log_file, NULL, _IOLBF, 0);
    
    return g_log_file;
}

/*
 * sec_log_init - Initialize security logging system
 */
int sec_log_init(const char *log_dir, int max_files, int max_size_mb) {
    if (log_dir) {
        strncpy(g_log_dir, log_dir, sizeof(g_log_dir) - 1);
        g_log_dir[sizeof(g_log_dir) - 1] = '\0';
    }
    
    if (max_files > 0) {
        g_max_files = max_files;
    }
    
    if (max_size_mb > 0) {
        g_max_size_bytes = max_size_mb * 1024 * 1024;
    }
    
    /* Create log directory if it doesn't exist */
    mkdir(g_log_dir, 0700);
    
    /* Initialize mutex */
    pthread_mutex_init(&g_log_lock, NULL);
    
    sys_info("Security log initialized: dir=%s, max_files=%d, max_size=%dMB\n",
             g_log_dir, g_max_files, max_size_mb);
    
    return 0;
}

/*
 * sec_log_event - Log a security event
 */
int sec_log_event(sec_log_level_t level, sec_event_type_t type,
                 const char *source_ip, const char *mac,
                 const char *username, const char *message,
                 const char *details) {
    if (!message) {
        return -1;
    }
    
    pthread_mutex_lock(&g_log_lock);
    
    /* Rotate logs if needed */
    rotate_logs();
    
    /* Open log file */
    FILE *fp = open_log_file();
    if (!fp) {
        pthread_mutex_unlock(&g_log_lock);
        return -1;
    }
    
    /* Get timestamp */
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    
    /* Write log entry */
    fprintf(fp, "%04d-%02d-%02d %02d:%02d:%02d [%s] [%s] ",
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec,
            level_str(level), event_type_str(type));
    
    if (source_ip && *source_ip) {
        fprintf(fp, "IP=%s ", source_ip);
    }
    if (mac && *mac) {
        fprintf(fp, "MAC=%s ", mac);
    }
    if (username && *username) {
        fprintf(fp, "USER=%s ", username);
    }
    
    fprintf(fp, "- %s", message);
    
    if (details && *details) {
        fprintf(fp, " [%s]", details);
    }
    
    fprintf(fp, "\n");
    
    pthread_mutex_unlock(&g_log_lock);
    
    return 0;
}

/*
 * sec_log_auth_failure - Log authentication failure
 */
int sec_log_auth_failure(const char *source_ip, const char *username,
                         const char *reason) {
    return sec_log_event(SEC_LOG_WARNING, SEC_EVENT_AUTH_FAILURE,
                         source_ip, NULL, username,
                         "Authentication failed", reason);
}

/*
 * sec_log_auth_success - Log successful authentication
 */
int sec_log_auth_success(const char *source_ip, const char *username) {
    return sec_log_event(SEC_LOG_INFO, SEC_EVENT_AUTH_SUCCESS,
                         source_ip, NULL, username,
                         "Authentication successful", NULL);
}

/*
 * sec_log_rate_limit - Log rate limit violation
 */
int sec_log_rate_limit(const char *source_ip, const char *mac, int type) {
    char details[64];
    snprintf(details, sizeof(details), "type=%s",
             type == 0 ? "registration" : "command");
    return sec_log_event(SEC_LOG_WARNING, SEC_EVENT_RATE_LIMIT,
                         source_ip, mac, NULL,
                         "Rate limit exceeded", details);
}

/*
 * sec_log_replay_attack - Log replay attack detection
 */
int sec_log_replay_attack(const char *source_ip, uint32_t random) {
    char details[32];
    snprintf(details, sizeof(details), "random=0x%08x", random);
    return sec_log_event(SEC_LOG_ALERT, SEC_EVENT_REPLAY_ATTACK,
                         source_ip, NULL, NULL,
                         "Replay attack detected", details);
}

/*
 * sec_log_ap_connect - Log AP connection
 */
int sec_log_ap_connect(const char *mac, const char *ip) {
    return sec_log_event(SEC_LOG_INFO, SEC_EVENT_AP_CONNECT,
                         ip, mac, NULL,
                         "AP connected", NULL);
}

/*
 * sec_log_ap_disconnect - Log AP disconnection
 */
int sec_log_ap_disconnect(const char *mac, const char *reason) {
    return sec_log_event(SEC_LOG_NOTICE, SEC_EVENT_AP_DISCONNECT,
                         NULL, mac, NULL,
                         "AP disconnected", reason);
}

/*
 * sec_log_config_change - Log configuration change
 */
int sec_log_config_change(const char *username, const char *change) {
    return sec_log_event(SEC_LOG_NOTICE, SEC_EVENT_CONFIG_CHANGE,
                         NULL, NULL, username,
                         "Configuration changed", change);
}

/*
 * sec_log_firmware_upgrade - Log firmware upgrade
 */
int sec_log_firmware_upgrade(const char *ap_mac, const char *version) {
    char details[64];
    snprintf(details, sizeof(details), "version=%s", version);
    return sec_log_event(SEC_LOG_INFO, SEC_EVENT_FIRMWARE_UPGRADE,
                         NULL, ap_mac, NULL,
                         "Firmware upgrade", details);
}

/*
 * sec_log_get_events - Retrieve security events
 */
int sec_log_get_events(sec_log_event_t *events, int max_events,
                       time_t start_time, time_t end_time,
                       sec_log_level_t level_filter) {
    if (!events || max_events <= 0) {
        return -1;
    }
    
    pthread_mutex_lock(&g_log_lock);
    
    char path[256];
    get_log_path(path, sizeof(path));
    
    FILE *fp = fopen(path, "r");
    if (!fp) {
        pthread_mutex_unlock(&g_log_lock);
        return 0;
    }
    
    char line[2048];
    int count = 0;
    
    while (fgets(line, sizeof(line), fp) && count < max_events) {
        /* Parse log line format: YYYY-MM-DD HH:MM:SS [LEVEL] [TYPE] ... */
        int year, month, day, hour, min, sec;
        char level_str[16], type_str[32];
        
        if (sscanf(line, "%d-%d-%d %d:%d:%d [%15s] [%31s]",
                   &year, &month, &day, &hour, &min, &sec,
                   level_str, type_str) >= 8) {
            
            time_t timestamp = mktime(&(struct tm){
                .tm_year = year - 1900,
                .tm_mon = month - 1,
                .tm_mday = day,
                .tm_hour = hour,
                .tm_min = min,
                .tm_sec = sec
            });
            
            /* Apply time filter */
            if ((start_time == 0 || timestamp >= start_time) &&
                (end_time == 0 || timestamp <= end_time)) {
                
                /* Apply level filter */
                sec_log_level_t level = SEC_LOG_DEBUG;
                for (int i = 0; i <= SEC_LOG_DEBUG; i++) {
                    if (strcmp(level_str, level_str((sec_log_level_t)i)) == 0) {
                        level = (sec_log_level_t)i;
                        break;
                    }
                }
                
                if (level <= level_filter) {
                    /* Parse remaining fields */
                    sec_event_type_t type = SEC_EVENT_SYSTEM_ERROR;
                    for (int i = 1; i <= SEC_EVENT_SYSTEM_ERROR; i++) {
                        if (strcmp(type_str, event_type_str((sec_event_type_t)i)) == 0) {
                            type = (sec_event_type_t)i;
                            break;
                        }
                    }
                    
                    events[count].timestamp = timestamp;
                    events[count].level = level;
                    events[count].type = type;
                    
                    /* Parse IP, MAC, USER from remaining line */
                    char *p = line;
                    while (*p && !(*p == 'I' && *(p+1) == 'P')) p++;
                    if (*p == 'I') {
                        p += 3;
                        char *end = strchr(p, ' ');
                        if (end) *end = '\0';
                        strncpy(events[count].source_ip, p, sizeof(events[count].source_ip) - 1);
                        if (end) *end = ' ';
                    }
                    
                    p = line;
                    while (*p && !(*p == 'M' && *(p+1) == 'A')) p++;
                    if (*p == 'M') {
                        p += 4;
                        char *end = strchr(p, ' ');
                        if (end) *end = '\0';
                        strncpy(events[count].mac_address, p, sizeof(events[count].mac_address) - 1);
                        if (end) *end = ' ';
                    }
                    
                    p = line;
                    while (*p && !(*p == 'U' && *(p+1) == 'S')) p++;
                    if (*p == 'U') {
                        p += 5;
                        char *end = strchr(p, ' ');
                        if (end) *end = '\0';
                        strncpy(events[count].username, p, sizeof(events[count].username) - 1);
                        if (end) *end = ' ';
                    }
                    
                    count++;
                }
            }
        }
    }
    
    fclose(fp);
    pthread_mutex_unlock(&g_log_lock);
    
    return count;
}

/*
 * sec_log_cleanup - Clean up old log files
 */
void sec_log_cleanup(void) {
    pthread_mutex_lock(&g_log_lock);
    
    if (g_log_file) {
        fclose(g_log_file);
        g_log_file = NULL;
    }
    
    pthread_mutex_unlock(&g_log_lock);
    pthread_mutex_destroy(&g_log_lock);
}