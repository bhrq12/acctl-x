/*
 * ============================================================================
 *
 *       Filename:  capwap_dtls.c
 *
 *    Description:  CAPWAP DTLS security implementation
 *                  Uses OpenSSL for DTLS encryption
 *
 *        Version:  1.0
 *        Created:  2026-04-26
 *       Revision:  initial version
 *       Compiler:  gcc
 *
 * ============================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/pem.h>

#include "log.h"
#include "capwap/capwap.h"

#define DTLS_MAX_PLAINTEXT_LEN 4096

/* DTLS context */
typedef struct capwap_dtls_ctx_t {
    SSL_CTX *ctx;
    SSL *ssl;
    BIO *bio;
    int sock;
    int is_server;
} capwap_dtls_ctx_t;

/* Initialize OpenSSL */
static int capwap_dtls_init_openssl(void)
{
    static int initialized = 0;
    
    if (!initialized) {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
        initialized = 1;
        sys_info("OpenSSL initialized\n");
    }
    
    return 0;
}

/* Create DTLS context */
static capwap_dtls_ctx_t *capwap_dtls_create_ctx(int is_server)
{
    capwap_dtls_ctx_t *dtls_ctx = (capwap_dtls_ctx_t *)malloc(sizeof(capwap_dtls_ctx_t));
    if (!dtls_ctx) {
        sys_err("capwap_dtls_create_ctx: malloc failed\n");
        return NULL;
    }
    
    memset(dtls_ctx, 0, sizeof(capwap_dtls_ctx_t));
    dtls_ctx->is_server = is_server;
    
    return dtls_ctx;
}

/* Load certificate and private key */
static int capwap_dtls_load_cert(capwap_dtls_ctx_t *ctx, const char *cert_file, const char *key_file)
{
    if (!ctx || !ctx->ctx) {
        return -1;
    }
    
    if (SSL_CTX_use_certificate_file(ctx->ctx, cert_file, SSL_FILETYPE_PEM) <= 0) {
        sys_err("capwap_dtls_load_cert: failed to load certificate\n");
        ERR_print_errors_fp(stderr);
        return -1;
    }
    
    if (SSL_CTX_use_PrivateKey_file(ctx->ctx, key_file, SSL_FILETYPE_PEM) <= 0) {
        sys_err("capwap_dtls_load_cert: failed to load private key\n");
        ERR_print_errors_fp(stderr);
        return -1;
    }
    
    if (!SSL_CTX_check_private_key(ctx->ctx)) {
        sys_err("capwap_dtls_load_cert: private key does not match certificate\n");
        return -1;
    }
    
    sys_info("Certificate and private key loaded\n");
    return 0;
}

/* Initialize DTLS for server */
int capwap_dtls_init_server(capwap_conn_t *conn, const char *cert_file, const char *key_file)
{
    if (!conn) {
        return -1;
    }
    
    capwap_dtls_init_openssl();
    
    capwap_dtls_ctx_t *dtls_ctx = capwap_dtls_create_ctx(1);
    if (!dtls_ctx) {
        return -1;
    }
    
    dtls_ctx->ctx = SSL_CTX_new(DTLS_method());
    if (!dtls_ctx->ctx) {
        sys_err("capwap_dtls_init_server: SSL_CTX_new failed\n");
        ERR_print_errors_fp(stderr);
        free(dtls_ctx);
        return -1;
    }
    
    /* Set security options */
    SSL_CTX_set_verify(dtls_ctx->ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    SSL_CTX_set_session_cache_mode(dtls_ctx->ctx, SSL_SESS_CACHE_OFF);
    
    /* Load certificate and key */
    if (cert_file && key_file) {
        if (capwap_dtls_load_cert(dtls_ctx, cert_file, key_file) < 0) {
            SSL_CTX_free(dtls_ctx->ctx);
            free(dtls_ctx);
            return -1;
        }
    }
    
    conn->dtls_session = dtls_ctx;
    sys_info("DTLS server initialized\n");
    return 0;
}

/* Certificate verification callback */
static int capwap_dtls_verify_callback(int preverify_ok, X509_STORE_CTX *ctx)
{
    if (preverify_ok) {
        return 1;
    }
    
    int err = X509_STORE_CTX_get_error(ctx);
    sys_warn("Certificate verification failed: %s\n", X509_verify_cert_error_string(err));
    return 0;
}

/* Initialize DTLS for client */
int capwap_dtls_init_client(capwap_conn_t *conn)
{
    if (!conn) {
        return -1;
    }
    
    capwap_dtls_init_openssl();
    
    capwap_dtls_ctx_t *dtls_ctx = capwap_dtls_create_ctx(0);
    if (!dtls_ctx) {
        return -1;
    }
    
    dtls_ctx->ctx = SSL_CTX_new(DTLS_method());
    if (!dtls_ctx->ctx) {
        sys_err("capwap_dtls_init_client: SSL_CTX_new failed\n");
        ERR_print_errors_fp(stderr);
        free(dtls_ctx);
        return -1;
    }
    
    /* Set security options */
    SSL_CTX_set_verify(dtls_ctx->ctx, SSL_VERIFY_PEER, capwap_dtls_verify_callback);
    SSL_CTX_set_session_cache_mode(dtls_ctx->ctx, SSL_SESS_CACHE_OFF);
    
    /* Load CA certificates - try default locations */
    if (SSL_CTX_load_verify_locations(dtls_ctx->ctx, NULL, "/etc/ssl/certs") != 1) {
        sys_warn("Failed to load CA certificates, using built-in ones\n");
    }
    
    conn->dtls_session = dtls_ctx;
    sys_info("DTLS client initialized with certificate verification\n");
    return 0;
}

/* Set socket for DTLS */
static int capwap_dtls_set_socket(capwap_dtls_ctx_t *ctx, int sock)
{
    if (!ctx || sock < 0) {
        return -1;
    }
    
    ctx->sock = sock;
    
    /* Create BIO for socket */
    ctx->bio = BIO_new_dgram(sock, BIO_NOCLOSE);
    if (!ctx->bio) {
        sys_err("capwap_dtls_set_socket: BIO_new_dgram failed\n");
        ERR_print_errors_fp(stderr);
        return -1;
    }
    
    /* Create SSL object */
    ctx->ssl = SSL_new(ctx->ctx);
    if (!ctx->ssl) {
        sys_err("capwap_dtls_set_socket: SSL_new failed\n");
        ERR_print_errors_fp(stderr);
        BIO_free(ctx->bio);
        return -1;
    }
    
    SSL_set_bio(ctx->ssl, ctx->bio, ctx->bio);
    
    if (ctx->is_server) {
        SSL_set_accept_state(ctx->ssl);
    } else {
        SSL_set_connect_state(ctx->ssl);
    }
    
    return 0;
}

/* Perform DTLS handshake */
int capwap_dtls_handshake(capwap_conn_t *conn)
{
    if (!conn || !conn->dtls_session || conn->ctrl_sock < 0) {
        return -1;
    }
    
    capwap_dtls_ctx_t *dtls_ctx = (capwap_dtls_ctx_t *)conn->dtls_session;
    
    if (!dtls_ctx->ssl) {
        if (capwap_dtls_set_socket(dtls_ctx, conn->ctrl_sock) < 0) {
            return -1;
        }
    }
    
    sys_info("Performing DTLS handshake\n");
    
    const int HANDSHAKE_TIMEOUT_SECONDS = 30;
    time_t start_time = time(NULL);
    int ret;
    
    while (1) {
        /* Check for timeout */
        if (time(NULL) - start_time > HANDSHAKE_TIMEOUT_SECONDS) {
            sys_err("capwap_dtls_handshake: handshake timeout after %d seconds\n", HANDSHAKE_TIMEOUT_SECONDS);
            return -1;
        }
        
        ret = SSL_do_handshake(dtls_ctx->ssl);
        if (ret == 1) {
            break;  /* Handshake successful */
        }
        
        int ssl_err = SSL_get_error(dtls_ctx->ssl, ret);
        if (ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE) {
            /* Small delay to avoid busy loop */
            usleep(10000);  /* 10ms */
            continue;
        }
        
        sys_err("capwap_dtls_handshake: handshake failed\n");
        ERR_print_errors_fp(stderr);
        return -1;
    }
    
    sys_info("DTLS handshake successful\n");
    return 0;
}

/* Encrypt data with DTLS */
int capwap_dtls_encrypt(capwap_conn_t *conn, char *data, int len, char *encrypted, int *encrypted_len)
{
    if (!conn || !conn->dtls_session || !data || !encrypted || !encrypted_len) {
        return -1;
    }

    capwap_dtls_ctx_t *dtls_ctx = (capwap_dtls_ctx_t *)conn->dtls_session;
    if (!dtls_ctx->ssl) {
        return -1;
    }

    const int MAX_RETRIES = 100;
    int retries = 0;

    while (retries < MAX_RETRIES) {
        int ret = SSL_write(dtls_ctx->ssl, data, len);
        if (ret > 0) {
            *encrypted_len = ret;
            return 0;
        }

        int ssl_err = SSL_get_error(dtls_ctx->ssl, ret);
        if (ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE) {
            usleep(10000);
            retries++;
            continue;
        }

        sys_err("capwap_dtls_encrypt: SSL_write failed\n");
        ERR_print_errors_fp(stderr);
        return -1;
    }

    sys_err("capwap_dtls_encrypt: max retries exceeded\n");
    return -1;
}

/* Decrypt data with DTLS */
int capwap_dtls_decrypt(capwap_conn_t *conn, char *encrypted, int len, char *data, int *data_len)
{
    if (!conn || !conn->dtls_session || !encrypted || !data || !data_len) {
        return -1;
    }

    capwap_dtls_ctx_t *dtls_ctx = (capwap_dtls_ctx_t *)conn->dtls_session;
    if (!dtls_ctx->ssl) {
        return -1;
    }

    int written = BIO_write(dtls_ctx->bio, encrypted, len);
    if (written != len) {
        sys_err("capwap_dtls_decrypt: BIO_write failed\n");
        return -1;
    }

    const int MAX_RETRIES = 100;
    int retries = 0;

    while (retries < MAX_RETRIES) {
        int ret = SSL_read(dtls_ctx->ssl, data, DTLS_MAX_PLAINTEXT_LEN - 1);
        if (ret > 0) {
            data[ret] = '\0';
            *data_len = ret;
            return 0;
        }

        int ssl_err = SSL_get_error(dtls_ctx->ssl, ret);
        if (ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE) {
            usleep(10000);
            retries++;
            continue;
        }

        sys_err("capwap_dtls_decrypt: SSL_read failed\n");
        ERR_print_errors_fp(stderr);
        return -1;
    }

    sys_err("capwap_dtls_decrypt: max retries exceeded\n");
    return -1;
}

/* Cleanup DTLS context */
void capwap_dtls_cleanup(capwap_conn_t *conn)
{
    if (!conn || !conn->dtls_session) {
        return;
    }
    
    capwap_dtls_ctx_t *dtls_ctx = (capwap_dtls_ctx_t *)conn->dtls_session;
    
    if (dtls_ctx->ssl) {
        SSL_shutdown(dtls_ctx->ssl);
        SSL_free(dtls_ctx->ssl);
    }
    
    if (dtls_ctx->bio) {
        BIO_free(dtls_ctx->bio);
    }
    
    if (dtls_ctx->ctx) {
        SSL_CTX_free(dtls_ctx->ctx);
    }
    
    free(dtls_ctx);
    conn->dtls_session = NULL;
    
    sys_info("DTLS context cleaned up\n");
}

/* Generate self-signed certificate */
int capwap_dtls_generate_cert(const char *cert_file, const char *key_file, const char *common_name)
{
    EVP_PKEY *pkey = NULL;
    X509 *x509 = NULL;
    X509_NAME *name = NULL;
    FILE *fp = NULL;
    EVP_PKEY_CTX *pkey_ctx = NULL;
    
    /* Validate parameters */
    if (!cert_file || !key_file || !common_name) {
        sys_err("capwap_dtls_generate_cert: invalid parameters\n");
        return -1;
    }
    
    /* Generate private key using EVP interface (OpenSSL 3.0 compatible) */
    pkey_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    if (!pkey_ctx) {
        sys_err("capwap_dtls_generate_cert: EVP_PKEY_CTX_new_id failed\n");
        goto error;
    }
    
    if (EVP_PKEY_keygen_init(pkey_ctx) <= 0) {
        sys_err("capwap_dtls_generate_cert: EVP_PKEY_keygen_init failed\n");
        goto error;
    }
    
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(pkey_ctx, 2048) <= 0) {
        sys_err("capwap_dtls_generate_cert: EVP_PKEY_CTX_set_rsa_keygen_bits failed\n");
        goto error;
    }
    
    if (EVP_PKEY_keygen(pkey_ctx, &pkey) <= 0) {
        sys_err("capwap_dtls_generate_cert: EVP_PKEY_keygen failed\n");
        goto error;
    }
    
    /* Generate certificate */
    x509 = X509_new();
    if (!x509) {
        sys_err("capwap_dtls_generate_cert: X509_new failed\n");
        goto error;
    }
    
    X509_set_version(x509, 2);
    ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);
    X509_gmtime_adj(X509_get_notBefore(x509), 0);
    X509_gmtime_adj(X509_get_notAfter(x509), 365 * 24 * 3600);  /* 1 year */
    
    if (X509_set_pubkey(x509, pkey) != 1) {
        sys_err("capwap_dtls_generate_cert: X509_set_pubkey failed\n");
        goto error;
    }
    
    /* Set subject and issuer */
    name = X509_get_subject_name(x509);
    if (!name) {
        sys_err("capwap_dtls_generate_cert: X509_get_subject_name failed\n");
        goto error;
    }
    
    if (X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char *)"CN", -1, -1, 0) != 1) {
        sys_err("capwap_dtls_generate_cert: failed to add country name\n");
        goto error;
    }
    
    if (X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (unsigned char *)"AC Controller", -1, -1, 0) != 1) {
        sys_err("capwap_dtls_generate_cert: failed to add organization name\n");
        goto error;
    }
    
    if (X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *)common_name, -1, -1, 0) != 1) {
        sys_err("capwap_dtls_generate_cert: failed to add common name\n");
        goto error;
    }
    
    if (X509_set_issuer_name(x509, name) != 1) {
        sys_err("capwap_dtls_generate_cert: X509_set_issuer_name failed\n");
        goto error;
    }
    
    /* Sign certificate */
    if (X509_sign(x509, pkey, EVP_sha256()) != 1) {
        sys_err("capwap_dtls_generate_cert: X509_sign failed\n");
        goto error;
    }
    
    /* Write certificate to file */
    fp = fopen(cert_file, "w");
    if (!fp) {
        sys_err("capwap_dtls_generate_cert: failed to open certificate file: %s\n", strerror(errno));
        goto error;
    }
    
    if (PEM_write_X509(fp, x509) != 1) {
        sys_err("capwap_dtls_generate_cert: failed to write certificate\n");
        fclose(fp);
        goto error;
    }
    fclose(fp);
    fp = NULL;
    
    /* Write private key to file */
    fp = fopen(key_file, "w");
    if (!fp) {
        sys_err("capwap_dtls_generate_cert: failed to open key file: %s\n", strerror(errno));
        goto error;
    }
    
    if (PEM_write_PrivateKey(fp, pkey, NULL, NULL, 0, NULL, NULL) != 1) {
        sys_err("capwap_dtls_generate_cert: failed to write private key\n");
        fclose(fp);
        goto error;
    }
    fclose(fp);
    fp = NULL;
    
    sys_info("Self-signed certificate generated: %s\n", cert_file);
    
    X509_free(x509);
    EVP_PKEY_free(pkey);
    if (pkey_ctx) EVP_PKEY_CTX_free(pkey_ctx);
    return 0;
    
error:
    if (x509) X509_free(x509);
    if (pkey) EVP_PKEY_free(pkey);
    if (pkey_ctx) EVP_PKEY_CTX_free(pkey_ctx);
    if (fp) fclose(fp);
    sys_err("capwap_dtls_generate_cert: failed\n");
    ERR_print_errors_fp(stderr);
    return -1;
}