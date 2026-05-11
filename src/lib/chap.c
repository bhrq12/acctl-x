/*
 * ============================================================================
 *
 *       Filename:  chap.c
 *
 *    Description:  CHAP authentication implementation for AC Controller
 *                  MD5-based challenge-response protocol
 *
 *        Version:  2.0
 *       Revision:  2026-04-15 — added chap_get_random, chap_get_md5_ex
 *       Compiler:  gcc
 *
 * ============================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>

#include "log.h"
#include "md5.h"
#include "msg.h"
#include "chap.h"

/* External dependencies */
extern int proc_cfgfile(char *path);

#define MAX_PASSWORD_LEN  128

/* Global password storage — populated from /etc/config/acctl-ac at runtime */
static char g_password[MAX_PASSWORD_LEN] = {0};
static int g_password_loaded = 0;

/*
 * md5 — wrapper for MD5 computation using internal MD5 library
 *   Outputs raw 16-byte digest into `digest` buffer
 */
static void md5(const void *data, int datalen, void *digest)
{
    if (!data || !digest || datalen <= 0) return;
    
    MD5_CTX ctx;
    MD5Init(&ctx);
    MD5Update(&ctx, (unsigned char *)data, (unsigned int)datalen);
    MD5Final(&ctx, (unsigned char *)digest);
}

/*
 * chap_md5 — compute MD5 digest of arbitrary binary data
 *   Outputs raw 16-byte digest into `digest` buffer (must be CHAP_LEN bytes)
 */
void chap_md5(const void *data, int datalen, void *digest)
{
    if (!data || !digest) return;
    md5(data, datalen, digest);
}

/*
 * chap_get_md5 — compute CHAP MD5 response
 *   response = MD5(chap_id + password + challenge)
 *   Implements RFC 1994 CHAP protocol.
 */
void chap_get_md5(const void *challenge, int challen, uint32_t chap_id,
                  const char *password, void *response)
{
    if (!challenge || !password || !response) return;
    if (challen <= 0) return;

    /* Build message: chap_id (1 byte) + password + challenge */
    size_t pwlen = strlen(password);
    size_t msglen = 1 + pwlen + (size_t)challen;
    unsigned char msg[1 + MAX_PASSWORD_LEN + 256];  /* Fixed size buffer */

    if (msglen > sizeof(msg)) {
        sys_warn("chap_get_md5: message too large\n");
        return;
    }

    msg[0] = (unsigned char)(chap_id & 0xFF);
    memcpy(msg + 1, password, pwlen);
    memcpy(msg + 1 + pwlen, challenge, (size_t)challen);

    chap_md5(msg, (int)msglen, response);
}

/*
 * chap_get_md5_ex — extended version taking chap_id as explicit uint32_t
 *   (avoids sign-extension issues when called from contexts with `int` random)
 */
void chap_get_md5_ex(const void *challenge, int challen, uint32_t chap_id,
                     const char *password, void *response)
{
    chap_get_md5(challenge, challen, chap_id, password, response);
}

/*
 * chap_cmp_md5 — verify MD5 challenge-response against known password
 *   Returns 0 on match (authentication success), -1 on failure.
 */
int chap_cmp_md5(const void *challenge, int challen, uint32_t chap_id,
                 const char *password, const void *response)
{
    if (!challenge || !password || !response) return -1;

    unsigned char computed[CHAP_LEN];
    chap_get_md5(challenge, challen, chap_id, password, computed);

    if (memcmp(computed, response, CHAP_LEN) == 0) {
        return 0;  /* Match */
    }
    return -1;     /* Mismatch */
}

/*
 * chap_fill_msg_md5 — compute CHAP MD5 and store in message header
 */
void chap_fill_msg_md5(struct msg_head_t *msg, int len, int random)
{
    if (!msg) return;
    chap_get_md5((void *)msg, len, (uint32_t)random, sec_get_password(), msg->chap);
}

/*
 * chap_get_random — get cryptographic random number from /dev/urandom
 *   Uses /dev/urandom (non-blocking) — safe for non-cryptographic use.
 *   For TLS keys, use sec_get_random_secure() instead.
 */
uint32_t chap_get_random(void)
{
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        sys_err("chap_get_random: open /dev/urandom failed: %s\n", strerror(errno));
        return 0;
    }

    uint32_t val;
    ssize_t ret = read(fd, &val, sizeof(val));
    close(fd);

    if (ret != sizeof(val)) {
        sys_err("chap_get_random: read failed, falling back to time\n");
        return (uint32_t)time(NULL);
    }

    return val;
}

/*
 * sec_get_password — retrieve the configured password
 *   Returns pointer to global password buffer.
 */
const char *sec_get_password(void)
{
    if (!g_password_loaded) {
        sys_warn("sec_get_password: password not yet loaded\n");
        return "";
    }
    return g_password;
}

/*
 * sec_password_check — verify password against stored value
 *   Returns 0 if password matches, -1 otherwise.
 */
int sec_password_check(void)
{
    /* For now, only UCI-configured passwords are supported.
     * Future versions may support PAM or other auth backends.
     */
    if (!g_password_loaded) {
        sys_warn("sec_password_check: no password configured\n");
        return -1;
    }
    return 0;
}

/*
 * chap_init_password — load password from UCI config file
 *   Scans /etc/config/acctl-ac for lines of the form:
 *     option password '<value>'
 *   Stops at first match.  Sets g_password_loaded = 1 on success.
 *
 *   Returns 0 on success, -1 on error.
 */
int chap_init_password(void)
{
    FILE *fp = fopen("/etc/config/acctl-ac", "r");
    if (!fp) {
        /* Try alternative path */
        fp = fopen("/etc/acctl-ac/ac.conf", "r");
        if (!fp) {
            sys_warn("chap_init_password: cannot open config: %s\n", strerror(errno));
            return -1;
        }
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        /* Skip leading whitespace */
        char *p = line;
        while (isspace((unsigned char)*p)) p++;

        /* Look for 'option password' */
        if (strncmp(p, "option", 5) == 0) {
            p += 5;
            while (isspace((unsigned char)*p)) p++;

            if (strncmp(p, "password", 8) == 0) {
                p += 8;
                while (isspace((unsigned char)*p)) p++;

                /* Expect '=' or whitespace separator */
                if (*p == '=' || *p == '\'') {
                    if (*p == '=') p++;
                    while (isspace((unsigned char)*p) || *p == '\'' || *p == '"') p++;

                    /* Extract value up to closing quote */
                    char *val = p;
                    char *end = strchr(val, '\'');
                    if (!end) end = strchr(val, '"');
                    if (end) {
                        size_t len = (size_t)(end - val);
                        if (len > MAX_PASSWORD_LEN - 1)
                            len = MAX_PASSWORD_LEN - 1;
                        strncpy(g_password, val, len);
                        g_password[len] = '\0';
                        fclose(fp);
                        g_password_loaded = 1;
                        sys_debug("Password loaded from UCI config\n");
                        return 0;
                    }
                }
            }
        }
    }

    fclose(fp);
    sys_warn("chap_init_password: no password found in config\n");
    return -1;
}

/*
 * chap_is_msg_authentic — verify incoming message CHAP digest
 *   Returns 0 if authentic, -1 if not.
 */
int chap_is_msg_authentic(struct msg_head_t *msg, int len)
{
    if (!msg) return -1;

    /* Compute expected digest using stored password */
    unsigned char computed[CHAP_LEN];
    chap_get_md5((void *)msg, len, (uint32_t)msg->random, sec_get_password(), computed);

    if (memcmp(computed, msg->chap, CHAP_LEN) != 0) {
        sys_warn("chap_is_msg_authentic: digest mismatch\n");
        return -1;
    }

    return 0;
}

/*
 * chap_msg_cmp_md5 — compare message MD5 digest against expected
 *   Returns 0 on match, -1 on failure.
 */
int chap_msg_cmp_md5(struct msg_head_t *msg, int len, uint32_t random)
{
    if (!msg) return -1;

    unsigned char computed[CHAP_LEN];
    chap_get_md5((void *)msg, len, random, sec_get_password(), computed);

    return memcmp(computed, msg->chap, CHAP_LEN);
}