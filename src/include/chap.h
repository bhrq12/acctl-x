/*
 * =====================================================================================
 *       Filename:  chap.h
 *       Description:  CHAP authentication declarations
 * =====================================================================================
 */
#ifndef __CHAP_H__
#define __CHAP_H__

#include <stdint.h>

#define CHAP_LEN  16

struct msg_head_t;

/* MD5 digest computation */
void chap_md5(const void *data, int datalen, void *digest);

/* CHAP MD5 challenge-response */
void chap_get_md5(const void *challenge, int challen, uint32_t chap_id,
                  const char *password, void *response);
void chap_get_md5_ex(const void *challenge, int challen, uint32_t chap_id,
                     const char *password, void *response);
int chap_cmp_md5(const void *challenge, int challen, uint32_t chap_id,
                 const char *password, const void *response);

/* Message-level CHAP operations */
int chap_msg_cmp_md5(struct msg_head_t *msg, int len, uint32_t random);
void chap_fill_msg_md5(struct msg_head_t *msg, int len, int random);
int chap_is_msg_authentic(struct msg_head_t *msg, int len);

/* Utility functions */
uint32_t chap_get_random(void);
int chap_init_password(void);

/* Security/password functions */
const char *sec_get_password(void);
int sec_password_check(void);

#endif /* __CHAP_H__ */