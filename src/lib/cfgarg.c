/*
 * ============================================================================
 *
 *       Filename:  cfgarg.c
 *
 *    Description:  UCI configuration file parser for AC Controller
 *                  Reads configuration from /etc/config/acctl-ac
 *
 *        Version:  2.0
 *        Created:  2014年08月19日 10时19分42秒
 *       Revision:  complete implementation for UCI config parsing
 *       Compiler:  gcc
 *
 *         Author:  jianxi sun (jianxi), ycsunjane@gmail.com
 *   Organization:  OpenWrt AC Controller Project
 *
 * ============================================================================
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include "arg.h"
#include "log.h"

#define CONFIG_FILE "/etc/config/acctl-ac"

/* Forward declaration of uci functions */
extern void uci_load_config(const char *config, void (*callback)(const char *section, const char *option, const char *value));

/* UCI config callback */
static void __uci_config_callback(const char *section, const char *option, const char *value)
{
    if (!section || !option || !value)
        return;

    sys_debug("UCI config: [%s] %s=%s\n", section, option, value);

    /* Parse 'ac' section options */
    if (strcmp(section, "ac") == 0) {
        if (strcmp(option, "port") == 0) {
            argument.port = atoi(value);
        } else if (strcmp(option, "nic") == 0) {
            strncpy(argument.nic, value, IFNAMSIZ - 1);
            argument.nic[IFNAMSIZ - 1] = '\0';
        } else if (strcmp(option, "brditv") == 0) {
#ifdef SERVER
            argument.brditv = atoi(value);
#endif
        } else if (strcmp(option, "reschkitv") == 0) {
#ifdef SERVER
            argument.reschkitv = atoi(value);
#endif
        } else if (strcmp(option, "msgitv") == 0) {
            argument.msgitv = atoi(value);
        } else if (strcmp(option, "debug") == 0) {
            debug = atoi(value);
        }
    }

    /* Parse 'network' section options */
    if (strcmp(section, "network") == 0) {
        if (strcmp(option, "ip_start") == 0) {
#ifdef SERVER
            strncpy(argument.ip_start, value, sizeof(argument.ip_start) - 1);
            argument.ip_start[sizeof(argument.ip_start) - 1] = '\0';
#endif
        } else if (strcmp(option, "ip_end") == 0) {
#ifdef SERVER
            strncpy(argument.ip_end, value, sizeof(argument.ip_end) - 1);
            argument.ip_end[sizeof(argument.ip_end) - 1] = '\0';
#endif
        } else if (strcmp(option, "ip_mask") == 0) {
#ifdef SERVER
            strncpy(argument.ip_mask, value, sizeof(argument.ip_mask) - 1);
            argument.ip_mask[sizeof(argument.ip_mask) - 1] = '\0';
#endif
        }
    }

    /* Parse 'client' section options (for AP mode) */
    if (strcmp(section, "client") == 0 || strcmp(section, "acctl-ap") == 0) {
        if (strcmp(option, "reportitv") == 0 || strcmp(option, "heartbeat_interval") == 0) {
            argument.reportitv = atoi(value);
        } else if (strcmp(option, "ac_ip") == 0) {
            argument.acaddr.sin_addr.s_addr = inet_addr(value);
        } else if (strcmp(option, "ac_port") == 0) {
            argument.acaddr.sin_port = htons((uint16_t)atoi(value));
            argument.port = atoi(value);
        }
    }
}

/* Parse UCI configuration file */
void proc_cfgarg(void)
{
    struct stat st;

    /* Check if config file exists */
    if (stat(CONFIG_FILE, &st) != 0) {
        sys_debug("Config file %s not found, using defaults\n", CONFIG_FILE);
        return;
    }

    /* Load UCI configuration */
    sys_debug("Loading configuration from %s\n", CONFIG_FILE);
    
    /* Simple config file parser for environments without libuci */
    FILE *fp = fopen(CONFIG_FILE, "r");
    if (!fp) {
        sys_warn("Cannot open config file %s: %s\n", CONFIG_FILE, strerror(errno));
        return;
    }

    char line[256];
    char current_section[64] = {0};

    while (fgets(line, sizeof(line), fp)) {
        /* Remove trailing newline */
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }

        /* Skip empty lines and comments */
        if (len == 0 || line[0] == '#' || line[0] == ';')
            continue;

        /* Parse section header */
        if (line[0] == '[' && line[len - 1] == ']') {
            strncpy(current_section, line + 1, sizeof(current_section) - 1);
            current_section[sizeof(current_section) - 1] = '\0';
            /* Remove closing bracket */
            len = strlen(current_section);
            if (len > 0 && current_section[len - 1] == ']') {
                current_section[len - 1] = '\0';
            }
            continue;
        }

        /* Parse option = value */
        char *eq = strchr(line, '=');
        if (eq && current_section[0]) {
            *eq = '\0';
            char *option = line;
            char *value = eq + 1;

            /* Trim leading/trailing whitespace from option */
            while (*option == ' ') option++;
            len = strlen(option);
            while (len > 0 && option[len - 1] == ' ') option[--len] = '\0';

            /* Trim leading/trailing whitespace and quotes from value */
            while (*value == ' ') value++;
            len = strlen(value);
            while (len > 0 && value[len - 1] == ' ') value[--len] = '\0';
            
            /* Remove surrounding quotes */
            if (len >= 2 && ((value[0] == '"' && value[len - 1] == '"') ||
                             (value[0] == '\'' && value[len - 1] == '\''))) {
                value[len - 1] = '\0';
                value++;
            }

            /* Call callback with parsed values */
            __uci_config_callback(current_section, option, value);
        }
    }

    fclose(fp);
    sys_info("Configuration loaded from %s\n", CONFIG_FILE);
}
