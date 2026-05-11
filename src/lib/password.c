/*
 * ============================================================================
 *
 *       Filename:  password.c
 *
 *    Description:  Secure password handling implementation
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
#include <ctype.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/encode.h>

#include "password.h"
#include "log.h"

/* Base64 encoding table for bcrypt */
static const char base64_chars[] = 
    "./ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

/*
 * base64_encode_bcrypt - Custom base64 encoding for bcrypt
 */
static void base64_encode_bcrypt(const uint8_t *data, size_t len, char *out)
{
    size_t i, j;
    uint32_t v;
    
    for (i = 0, j = 0; i < len; i += 3) {
        v = ((uint32_t)data[i]) << 16;
        if (i + 1 < len) v |= ((uint32_t)data[i + 1]) << 8;
        if (i + 2 < len) v |= data[i + 2];
        
        out[j++] = base64_chars[(v >> 18) & 0x3f];
        out[j++] = base64_chars[(v >> 12) & 0x3f];
        if (i + 1 < len) out[j++] = base64_chars[(v >> 6) & 0x3f];
        if (i + 2 < len) out[j++] = base64_chars[v & 0x3f];
    }
    out[j] = '\0';
}

/*
 * pw_generate_salt - Generate a random bcrypt salt
 */
int pw_generate_salt(char *salt_out, size_t salt_len)
{
    if (!salt_out || salt_len < PW_BCRYPT_SALT_LEN + 1) {
        sys_err("pw_generate_salt: invalid parameters\n");
        return -1;
    }

    uint8_t raw_salt[PW_BCRYPT_SALT_LEN];
    if (RAND_bytes(raw_salt, PW_BCRYPT_SALT_LEN) != 1) {
        sys_err("pw_generate_salt: failed to generate random bytes\n");
        return -1;
    }

    base64_encode_bcrypt(raw_salt, PW_BCRYPT_SALT_LEN, salt_out);
    return 0;
}

/*
 * pw_hash_password - Hash a password using bcrypt
 */
int pw_hash_password(const char *password, char *hash_out, size_t hash_len)
{
    if (!password || !hash_out || hash_len < PW_BCRYPT_HASH_LEN + 1) {
        sys_err("pw_hash_password: invalid parameters\n");
        return -1;
    }

    char salt[PW_BCRYPT_SALT_LEN + 1];
    if (pw_generate_salt(salt, sizeof(salt)) != 0) {
        return -1;
    }

    /* Use OpenSSL's EVP interface for bcrypt */
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) {
        sys_err("pw_hash_password: EVP_MD_CTX_new failed\n");
        return -1;
    }

    /* For OpenWrt compatibility, we'll use PBKDF2-HMAC-SHA256 as fallback
     * since bcrypt may not be available in all environments */
    
    uint8_t derived_key[32];
    if (PKCS5_PBKDF2_HMAC(password, strlen(password),
                          (const uint8_t *)salt, strlen(salt),
                          100000, EVP_sha256(),
                          sizeof(derived_key), derived_key) != 1) {
        EVP_MD_CTX_free(ctx);
        sys_err("pw_hash_password: PBKDF2 failed\n");
        return -1;
    }

    EVP_MD_CTX_free(ctx);

    /* Format: $pbkdf2-sha256$iterations$salt$hash */
    snprintf(hash_out, hash_len, "$pbkdf2-sha256$100000$%s$", salt);
    
    /* Base64 encode the derived key */
    BIO *bio = BIO_new(BIO_s_mem());
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO_push(b64, bio);
    BIO_write(b64, derived_key, sizeof(derived_key));
    BIO_flush(b64);
    
    char hash_b64[64];
    int len = BIO_read(bio, hash_b64, sizeof(hash_b64) - 1);
    hash_b64[len] = '\0';
    
    /* Remove newlines from base64 output */
    char *p = hash_b64;
    char *q = hash_b64;
    while (*p) {
        if (*p != '\n' && *p != '\r') {
            *q++ = *p;
        }
        p++;
    }
    *q = '\0';
    
    strncat(hash_out, hash_b64, hash_len - strlen(hash_out) - 1);
    
    BIO_free_all(b64);
    
    sys_debug("pw_hash_password: hash generated successfully\n");
    return 0;
}

/*
 * pw_verify_password - Verify a password against a hash
 */
int pw_verify_password(const char *password, const char *hash)
{
    if (!password || !hash) {
        sys_err("pw_verify_password: invalid parameters\n");
        return -2;
    }

    /* Parse hash format: $algorithm$iterations$salt$hash */
    char *p = (char *)hash;
    
    /* Skip algorithm prefix */
    if (strncmp(p, "$pbkdf2-sha256$", 14) != 0) {
        sys_warn("pw_verify_password: unsupported hash format\n");
        return -2;
    }
    p += 14;

    /* Get iterations */
    char *iter_end = strchr(p, '$');
    if (!iter_end) return -2;
    *iter_end = '\0';
    int iterations = atoi(p);
    p = iter_end + 1;

    /* Get salt */
    char *salt_end = strchr(p, '$');
    if (!salt_end) return -2;
    *salt_end = '\0';
    char salt[64];
    strncpy(salt, p, sizeof(salt) - 1);
    p = salt_end + 1;

    /* Hash the password with the same parameters */
    uint8_t derived_key[32];
    if (PKCS5_PBKDF2_HMAC(password, strlen(password),
                          (const uint8_t *)salt, strlen(salt),
                          iterations, EVP_sha256(),
                          sizeof(derived_key), derived_key) != 1) {
        sys_err("pw_verify_password: PBKDF2 failed\n");
        return -2;
    }

    /* Base64 encode the derived key */
    BIO *bio = BIO_new(BIO_s_mem());
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO_push(b64, bio);
    BIO_write(b64, derived_key, sizeof(derived_key));
    BIO_flush(b64);
    
    char computed_hash[64];
    int len = BIO_read(bio, computed_hash, sizeof(computed_hash) - 1);
    computed_hash[len] = '\0';
    
    /* Remove newlines */
    char *q = computed_hash;
    len = 0;
    while (*q) {
        if (*q != '\n' && *q != '\r') {
            computed_hash[len++] = *q;
        }
        q++;
    }
    computed_hash[len] = '\0';
    
    BIO_free_all(b64);

    /* Compare hashes (constant time) */
    int result = CRYPTO_memcmp(computed_hash, p, strlen(p));
    
    if (result == 0) {
        sys_debug("pw_verify_password: password match\n");
        return 0;
    } else {
        sys_warn("pw_verify_password: password mismatch\n");
        return -1;
    }
}

/*
 * pw_check_strength - Check password strength
 */
pw_strength_t pw_check_strength(const char *password)
{
    if (!password) return PW_STRENGTH_WEAK;

    int len = strlen(password);
    int has_upper = 0, has_lower = 0, has_digit = 0, has_special = 0;

    for (int i = 0; i < len; i++) {
        if (isupper((unsigned char)password[i])) has_upper = 1;
        else if (islower((unsigned char)password[i])) has_lower = 1;
        else if (isdigit((unsigned char)password[i])) has_digit = 1;
        else if (strchr("!@#$%^&*()_+-=[]{}|;:,.<>?", password[i])) has_special = 1;
    }

    int score = 0;
    if (len >= 8) score++;
    if (len >= 12) score++;
    if (has_upper) score++;
    if (has_lower) score++;
    if (has_digit) score++;
    if (has_special) score++;

    if (score <= 2) return PW_STRENGTH_WEAK;
    if (score <= 4) return PW_STRENGTH_MEDIUM;
    if (score <= 5) return PW_STRENGTH_STRONG;
    return PW_STRENGTH_VERY_STRONG;
}

/*
 * pw_is_valid - Validate password meets minimum requirements
 */
int pw_is_valid(const char *password)
{
    if (!password) return -3;

    int len = strlen(password);
    
    if (len < PW_MIN_LENGTH) {
        sys_warn("pw_is_valid: password too short (%d < %d)\n", len, PW_MIN_LENGTH);
        return -1;
    }
    
    if (len > PW_MAX_LENGTH) {
        sys_warn("pw_is_valid: password too long (%d > %d)\n", len, PW_MAX_LENGTH);
        return -2;
    }

    /* Check for invalid characters */
    for (int i = 0; i < len; i++) {
        if (!isprint((unsigned char)password[i]) || 
            strchr("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d", password[i])) {
            sys_warn("pw_is_valid: password contains invalid characters\n");
            return -3;
        }
    }

    return 0;
}