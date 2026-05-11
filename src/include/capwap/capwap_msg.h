/*
 * ============================================================================
 *
 *       Filename:  capwap_msg.h
 *
 *    Description:  CAPWAP message header declarations
 *
 *        Version:  1.0
 *        Created:  2026-04-26
 *       Revision:  initial version
 *       Compiler:  gcc
 *
 * ============================================================================
 */
#ifndef __CAPWAP_MSG_H__
#define __CAPWAP_MSG_H__

#include <stdint.h>

#include "capwap.h"

/* CAPWAP message header structure (RFC 5415) */
typedef struct capwap_message_header_t {
    uint8_t version;
    uint8_t type;
    uint8_t flags;
    uint8_t reserved;
    uint16_t length;
    uint16_t fragment_id;
    uint8_t  fragment_offset;
    uint8_t  reserved2;
    uint32_t session_id;
} __attribute__((packed)) capwap_message_header_t;

/* CAPWAP element header */
typedef struct capwap_element_header_t {
    uint16_t type;
    uint16_t length;
} __attribute__((packed)) capwap_element_header_t;

/* CAPWAP parsed message */
typedef struct capwap_parsed_message_t {
    uint8_t type;
    uint32_t session_id;
    uint16_t fragment_id;
    uint8_t *payload;
    int payload_len;
} capwap_parsed_message_t;

/* CAPWAP result code */
enum capwap_result_code {
    CAPWAP_RESULT_SUCCESS = 0,
    CAPWAP_RESULT_FAILURE = 1,
    CAPWAP_RESULT_SYNTAX_ERROR = 2,
    CAPWAP_RESULT_UNRECOGNIZED_REQ = 3,
    CAPWAP_RESULT_AUTH_REQUIRED = 4,
};

/* CAPWAP element types (RFC 5415) */
enum capwap_element_type {
    CAPWAP_ELEMENT_AC_NAME = 1,
    CAPWAP_ELEMENT_WTP_NAME = 2,
    CAPWAP_ELEMENT_MAC_TYPE = 3,
    CAPWAP_ELEMENT_MAC_ADDRESS = 4,
    CAPWAP_ELEMENT_WTP_BOARD_DATA = 5,
    CAPWAP_ELEMENT_WTP_DESCRIPTOR = 6,
    CAPWAP_ELEMENT_LOCATION_DATA = 7,
    CAPWAP_ELEMENT_CAPABILITY = 8,
    CAPWAP_ELEMENT_STATUS = 9,
    CAPWAP_ELEMENT_RADIO_INFO = 10,
    CAPWAP_ELEMENT_STATISTICS = 11,
    CAPWAP_ELEMENT_REBOOT_STATISTICS = 12,
    CAPWAP_ELEMENT_EVENT = 13,
    CAPWAP_ELEMENT_CONTROL_IP_ADDRESS = 14,
    CAPWAP_ELEMENT_DATA_TRANSFER_MODE = 15,
    CAPWAP_ELEMENT_TIMERS = 16,
    CAPWAP_ELEMENT_AC_ID = 17,
    CAPWAP_ELEMENT_WTP_ID = 18,
    CAPWAP_ELEMENT_VENDOR_SPECIFIC = 255,
};

/* Build CAPWAP Discovery Request */
int capwap_build_discovery_request(char *buf, int *len, const char *wtp_name, const uint8_t *mac);

/* Build CAPWAP Join Request */
int capwap_build_join_request(char *buf, int *len, const char *wtp_name, const uint8_t *mac, uint32_t session_id);

/* Build CAPWAP Echo Request */
int capwap_build_echo_request(char *buf, int *len, uint32_t session_id);

/* Build CAPWAP WTP Event Request */
int capwap_build_wtp_event(char *buf, int *len, uint16_t event_type, void *event_data, int event_data_len, uint32_t session_id);

/* Build CAPWAP Configure Request */
int capwap_build_configure_request(char *buf, int *len, uint32_t session_id, uint8_t radio_id, const char *config_data, int config_len);

/* Build CAPWAP Reset Request */
int capwap_build_reset_request(char *buf, int *len, uint32_t session_id);

/* Build CAPWAP Statistics Request */
int capwap_build_statistics_request(char *buf, int *len, uint32_t session_id, uint8_t radio_id);

/* Parse CAPWAP message header */
int capwap_parse_header(const char *buf, int len, capwap_message_header_t *header);

/* Parse CAPWAP message elements */
int capwap_parse_elements(const char *buf, int len, int (*element_handler)(uint16_t type, const char *data, int data_len, void *arg), void *arg);

/* Parse complete CAPWAP message */
int capwap_parse_message(const char *buf, int len, capwap_parsed_message_t *parsed_msg);

/* Free parsed message */
void capwap_free_parsed_message(capwap_parsed_message_t *parsed_msg);

/* Get message type string */
const char *capwap_get_message_type_string(uint8_t type);

/* Message mapping functions */
int msg_map_to_capwap(int msg_type, char *data, int len, char *capwap_buf, int *capwap_len, uint32_t session_id);
int msg_map_from_capwap(const char *capwap_buf, int capwap_len, int *msg_type, char *data, int *len);

/* Build CAPWAP Image Data Request */
int capwap_build_image_data_request(char *buf, int *len, uint32_t session_id, uint32_t image_id,
                                    const char *image_data, int data_len, int offset);

/* Build CAPWAP Image Data Response */
int capwap_build_image_data_response(char *buf, int *len, uint32_t session_id, uint32_t image_id,
                                    int status, int offset, int total_size);

/* Image transfer functions */
void capwap_image_transfer_init(uint32_t image_id, uint32_t total_size);
int capwap_image_add_chunk(uint32_t offset, const char *data, int data_len);
char *capwap_image_get_data(uint32_t *total_len);
int capwap_image_is_complete(void);
void capwap_image_transfer_clear(void);

/* Fragmentation and reassembly functions */
int capwap_fragment_message(const char *data, int len, char **fragments, int *fragment_count, int max_frag_size);
int capwap_reassemble_message(capwap_message_header_t *header, const char *payload, int payload_len);
char *capwap_get_reassembled_data(int *len);
void capwap_clear_reassembly(void);

/* Retransmission functions */
int capwap_add_retransmit(uint8_t msg_type, char *buf, int len);
int capwap_check_retransmit(capwap_conn_t *conn);
void capwap_remove_retransmit(uint8_t msg_type);

/* Keepalive functions */
void capwap_stop_keepalive(void);

#endif /* __CAPWAP_MSG_H__ */