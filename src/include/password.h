/*
 * ============================================================================
 *
 *       Filename:  password.h
 *
 *    Description:  Secure password handling with bcrypt
 *
 *        Version:  1.0
 *        Created:  2026-04-29
 *       Compiler:  gcc
 *
 * ============================================================================
 */
#ifndef __PASSWORD_H__
#define __PASSWORD_H__

#include <stdint.h>

/* Maximum password length */
#define PW_MAX_LENGTH    128
#define PW_MIN_LENGTH    8

/* Bcrypt constants */
#define PW_BCRYPT_COST    12
#define PW_BCRYPT_SALT_LEN 16
#define PW_BCRYPT_HASH_LEN 60

/* Password strength levels */
typedef enum {
    PW_STRENGTH_WEAK = 0,
    PW_STRENGTH_MEDIUM = 1,
    PW_STRENGTH_STRONG = 2,
    PW_STRENGTH_VERY_STRONG = 3
} pw_strength_t;

/*
 * pw_hash_password - Hash a password using bcrypt
 * 
 * Parameters:
 *   password - Plaintext password to hash
 *   hash_out - Output buffer for the hash (must be at least PW_BCRYPT_HASH_LEN + 1)
 *   hash_len - Length of hash_out buffer
 *
 * Returns:
 *   0 on success, -1 on error
 */
int pw_hash_password(const char *password, char *hash_out, size_t hash_len);

/*
 * pw_verify_password - Verify a password against a bcrypt hash
 * 
 * Parameters:
 *   password - Plaintext password to verify
 *   hash - Stored bcrypt hash
 *
 * Returns:
 *   0 if password matches, -1 on mismatch, -2 on error
 */
int pw_verify_password(const char *password, const char *hash);

/*
 * pw_check_strength - Check password strength
 * 
 * Parameters:
 *   password - Password to check
 *
 * Returns:
 *   PW_STRENGTH_WEAK, PW_STRENGTH_MEDIUM, PW_STRENGTH_STRONG, or PW_STRENGTH_VERY_STRONG
 */
pw_strength_t pw_check_strength(const char *password);

/*
 * pw_is_valid - Validate password meets minimum requirements
 * 
 * Parameters:
 *   password - Password to validate
 *
 * Returns:
 *   0 if valid, -1 if too short, -2 if too long, -3 if invalid characters
 */
int pw_is_valid(const char *password);

/*
 * pw_generate_salt - Generate a random bcrypt salt
 * 
 * Parameters:
 *   salt_out - Output buffer for salt (must be at least PW_BCRYPT_SALT_LEN + 1)
 *   salt_len - Length of salt_out buffer
 *
 * Returns:
 *   0 on success, -1 on error
 */
int pw_generate_salt(char *salt_out, size_t salt_len);

#endif /* __PASSWORD_H__ */