/*
 * ============================================================================
 *
 *       Filename:  capwap.h
 *
 *    Description:  CAPWAP protocol integration header
 *
 *        Version:  1.0
 *        Created:  2026-04-26
 *       Revision:  initial version
 *       Compiler:  gcc
 *
 * ============================================================================
 */
#ifndef __CAPWAP_H__
#define __CAPWAP_H__

#include <stdint.h>
#include <netinet/in.h>

/* CAPWAP protocol constants */
#define CAPWAP_PORT_CTRL    5246  /* Control channel UDP port */
#define CAPWAP_PORT_DATA    5247  /* Data channel UDP port */

/* CAPWAP message types */
enum {
    /* Discovery */
    CAPWAP_MSG_DISCOVERY_REQUEST  = 1,
    CAPWAP_MSG_DISCOVERY_RESPONSE = 2,
    
    /* Join */
    CAPWAP_MSG_JOIN_REQUEST       = 3,
    CAPWAP_MSG_JOIN_RESPONSE      = 4,
    
    /* Configuration */
    CAPWAP_MSG_CONFIGURE_REQUEST  = 5,
    CAPWAP_MSG_CONFIGURE_RESPONSE = 6,
    
    /* Echo */
    CAPWAP_MSG_ECHO_REQUEST       = 7,
    CAPWAP_MSG_ECHO_RESPONSE      = 8,
    
    /* Reset */
    CAPWAP_MSG_RESET_REQUEST       = 9,
    CAPWAP_MSG_RESET_RESPONSE      = 10,
    
    /* Data */
    CAPWAP_MSG_DATA                = 11,
    
    /* WTP Event */
    CAPWAP_MSG_WTP_EVENT_REQUEST   = 12,
    
    /* Image Data */
    CAPWAP_MSG_IMAGE_DATA_REQUEST  = 13,
    CAPWAP_MSG_IMAGE_DATA_RESPONSE = 14,
    
    /* Statistics */
    CAPWAP_MSG_STATISTICS_REQUEST  = 15,
    CAPWAP_MSG_STATISTICS_RESPONSE = 16,
};

/* CAPWAP state machine */
enum capwap_state {
    CAPWAP_STATE_IDLE,
    CAPWAP_STATE_DISCOVERY,
    CAPWAP_STATE_JOIN,
    CAPWAP_STATE_CONFIGURE,
    CAPWAP_STATE_RUN,
};

/* CAPWAP connection struct */
typedef struct capwap_conn_t {
    int ctrl_sock;             /* Control channel socket */
    int data_sock;             /* Data channel socket */
    struct sockaddr_in ac_addr; /* AC address */
    struct sockaddr_in wtp_addr; /* WTP address */
    enum capwap_state state;   /* Current state */
    uint32_t sequence;         /* Message sequence number */
    uint32_t session_id;       /* Session ID */
    void *dtls_session;        /* DTLS session handle */
} capwap_conn_t;

/* Function prototypes */

/* CAPWAP connection management */
int capwap_init(capwap_conn_t *conn);
int capwap_connect(capwap_conn_t *conn, const char *ac_ip, int ac_port);
int capwap_disconnect(capwap_conn_t *conn);

/* CAPWAP message handling */
int capwap_send_discovery(capwap_conn_t *conn);
int capwap_send_join(capwap_conn_t *conn);
int capwap_send_configure(capwap_conn_t *conn);
int capwap_send_echo(capwap_conn_t *conn);
int capwap_send_reset(capwap_conn_t *conn);
int capwap_send_statistics(capwap_conn_t *conn, uint8_t radio_id);
int capwap_send_event(capwap_conn_t *conn, uint16_t event_type, void *event_data);

/* CAPWAP message processing */
int capwap_process_message(capwap_conn_t *conn, char *data, int len);

/* CAPWAP state management */
enum capwap_state capwap_get_state(capwap_conn_t *conn);

/* CAPWAP data channel */
int capwap_setup_data_channel(capwap_conn_t *conn);

/* CAPWAP keepalive */
void capwap_start_keepalive(capwap_conn_t *conn);

/* DTLS functions */
int capwap_dtls_init_server(capwap_conn_t *conn, const char *cert_file, const char *key_file);
int capwap_dtls_init_client(capwap_conn_t *conn);
int capwap_dtls_handshake(capwap_conn_t *conn);
int capwap_dtls_encrypt(capwap_conn_t *conn, char *data, int len, char *encrypted, int *encrypted_len);
int capwap_dtls_decrypt(capwap_conn_t *conn, char *encrypted, int len, char *data, int *data_len);
void capwap_dtls_cleanup(capwap_conn_t *conn);
int capwap_dtls_generate_cert(const char *cert_file, const char *key_file, const char *common_name);
int capwap_dtls_init_server_wrapper(capwap_conn_t *conn, const char *cert_file, const char *key_file);
int capwap_dtls_init_client_wrapper(capwap_conn_t *conn);
int capwap_dtls_handshake_wrapper(capwap_conn_t *conn);
int capwap_dtls_encrypt_wrapper(capwap_conn_t *conn, char *data, int len, char *encrypted, int *encrypted_len);
int capwap_dtls_decrypt_wrapper(capwap_conn_t *conn, char *encrypted, int len, char *data, int *data_len);
int capwap_dtls_generate_cert_wrapper(const char *cert_file, const char *key_file, const char *common_name);

#endif /* __CAPWAP_H__ */