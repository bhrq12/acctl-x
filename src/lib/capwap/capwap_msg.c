/*
 * ============================================================================
 *
 *       Filename:  capwap_msg.c
 *
 *    Description:  CAPWAP message implementation
 *                  Handles standard CAPWAP messages
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
#include <stdint.h>

#include "log.h"
#include "capwap/capwap.h"
#include "capwap/capwap_msg.h"
#include "msg.h"

/* Build CAPWAP message header */
static int capwap_build_header(capwap_message_header_t *header, uint8_t type, uint16_t length, uint32_t session_id)
{
    if (!header) {
        return -1;
    }

    memset(header, 0, sizeof(capwap_message_header_t));
    header->version = 0x02;  /* CAPWAP version 2 */
    header->type = type;
    header->flags = 0;       /* No flags set */
    header->length = htons(length);
    header->fragment_id = 0;
    header->fragment_offset = 0;
    header->session_id = htonl(session_id);

    return 0;
}

/* Build CAPWAP Discovery Request */
int capwap_build_discovery_request(char *buf, int *len, const char *wtp_name, const uint8_t *mac)
{
    if (!buf || !len) {
        return -1;
    }

    capwap_message_header_t header;
    if (capwap_build_header(&header, CAPWAP_MSG_DISCOVERY_REQUEST, 0, 0) < 0) {
        return -1;
    }

    /* Calculate message length */
    int msg_len = sizeof(capwap_message_header_t);

    /* Add WTP Name element if provided */
    if (wtp_name) {
        int name_len = strlen(wtp_name);
        capwap_element_header_t elem_hdr;
        elem_hdr.type = htons(CAPWAP_ELEMENT_WTP_NAME);
        elem_hdr.length = htons(name_len);

        memcpy(buf + msg_len, &elem_hdr, sizeof(elem_hdr));
        msg_len += sizeof(elem_hdr);
        memcpy(buf + msg_len, wtp_name, name_len);
        msg_len += name_len;
    }

    /* Add MAC Address element if provided */
    if (mac) {
        capwap_element_header_t elem_hdr;
        elem_hdr.type = htons(CAPWAP_ELEMENT_MAC_ADDRESS);
        elem_hdr.length = htons(6);  /* MAC address length */

        memcpy(buf + msg_len, &elem_hdr, sizeof(elem_hdr));
        msg_len += sizeof(elem_hdr);
        memcpy(buf + msg_len, mac, 6);
        msg_len += 6;
    }

    /* Update header length */
    header.length = htons(msg_len - sizeof(capwap_message_header_t));
    memcpy(buf, &header, sizeof(header));

    *len = msg_len;
    return 0;
}

/* Build CAPWAP Join Request */
int capwap_build_join_request(char *buf, int *len, const char *wtp_name, const uint8_t *mac, uint32_t session_id)
{
    if (!buf || !len) {
        return -1;
    }

    capwap_message_header_t header;
    if (capwap_build_header(&header, CAPWAP_MSG_JOIN_REQUEST, 0, session_id) < 0) {
        return -1;
    }

    /* Calculate message length */
    int msg_len = sizeof(capwap_message_header_t);

    /* Add WTP Name element */
    if (wtp_name) {
        int name_len = strlen(wtp_name);
        capwap_element_header_t elem_hdr;
        elem_hdr.type = htons(CAPWAP_ELEMENT_WTP_NAME);
        elem_hdr.length = htons(name_len);

        memcpy(buf + msg_len, &elem_hdr, sizeof(elem_hdr));
        msg_len += sizeof(elem_hdr);
        memcpy(buf + msg_len, wtp_name, name_len);
        msg_len += name_len;
    }

    /* Add MAC Address element */
    if (mac) {
        capwap_element_header_t elem_hdr;
        elem_hdr.type = htons(CAPWAP_ELEMENT_MAC_ADDRESS);
        elem_hdr.length = htons(6);

        memcpy(buf + msg_len, &elem_hdr, sizeof(elem_hdr));
        msg_len += sizeof(elem_hdr);
        memcpy(buf + msg_len, mac, 6);
        msg_len += 6;
    }

    /* Update header length */
    header.length = htons(msg_len - sizeof(capwap_message_header_t));
    memcpy(buf, &header, sizeof(header));

    *len = msg_len;
    return 0;
}

/* Build CAPWAP Echo Request */
int capwap_build_echo_request(char *buf, int *len, uint32_t session_id)
{
    if (!buf || !len) {
        return -1;
    }

    capwap_message_header_t header;
    if (capwap_build_header(&header, CAPWAP_MSG_ECHO_REQUEST, 0, session_id) < 0) {
        return -1;
    }

    int msg_len = sizeof(capwap_message_header_t);
    header.length = htons(msg_len - sizeof(capwap_message_header_t));
    memcpy(buf, &header, sizeof(header));

    *len = msg_len;
    return 0;
}

/* Build CAPWAP WTP Event Request */
int capwap_build_wtp_event(char *buf, int *len, uint16_t event_type, void *event_data, int event_data_len, uint32_t session_id)
{
    if (!buf || !len) {
        return -1;
    }

    capwap_message_header_t header;
    if (capwap_build_header(&header, CAPWAP_MSG_WTP_EVENT_REQUEST, 0, session_id) < 0) {
        return -1;
    }

    /* Calculate message length */
    int msg_len = sizeof(capwap_message_header_t);

    /* Add Event element */
    capwap_element_header_t elem_hdr;
    elem_hdr.type = htons(CAPWAP_ELEMENT_EVENT);
    elem_hdr.length = htons(2 + event_data_len);  /* Event type + data */

    memcpy(buf + msg_len, &elem_hdr, sizeof(elem_hdr));
    msg_len += sizeof(elem_hdr);
    memcpy(buf + msg_len, &event_type, 2);
    msg_len += 2;
    if (event_data && event_data_len > 0) {
        memcpy(buf + msg_len, event_data, event_data_len);
        msg_len += event_data_len;
    }

    /* Update header length */
    header.length = htons(msg_len - sizeof(capwap_message_header_t));
    memcpy(buf, &header, sizeof(header));

    *len = msg_len;
    return 0;
}

/* Build CAPWAP Configure Request */
int capwap_build_configure_request(char *buf, int *len, uint32_t session_id, uint8_t radio_id, const char *config_data, int config_len)
{
    if (!buf || !len) {
        return -1;
    }

    capwap_message_header_t header;
    if (capwap_build_header(&header, CAPWAP_MSG_CONFIGURE_REQUEST, 0, session_id) < 0) {
        return -1;
    }

    /* Calculate message length */
    int msg_len = sizeof(capwap_message_header_t);

    /* Add Radio Info element */
    capwap_element_header_t elem_hdr;
    elem_hdr.type = htons(CAPWAP_ELEMENT_RADIO_INFO);
    elem_hdr.length = htons(1);

    memcpy(buf + msg_len, &elem_hdr, sizeof(elem_hdr));
    msg_len += sizeof(elem_hdr);
    memcpy(buf + msg_len, &radio_id, 1);
    msg_len += 1;

    /* Add configuration data if provided */
    if (config_data && config_len > 0) {
        elem_hdr.type = htons(CAPWAP_ELEMENT_VENDOR_SPECIFIC);
        elem_hdr.length = htons(config_len);

        memcpy(buf + msg_len, &elem_hdr, sizeof(elem_hdr));
        msg_len += sizeof(elem_hdr);
        memcpy(buf + msg_len, config_data, config_len);
        msg_len += config_len;
    }

    /* Update header length */
    header.length = htons(msg_len - sizeof(capwap_message_header_t));
    memcpy(buf, &header, sizeof(header));

    *len = msg_len;
    return 0;
}

/* Build CAPWAP Reset Request */
int capwap_build_reset_request(char *buf, int *len, uint32_t session_id)
{
    if (!buf || !len) {
        return -1;
    }

    capwap_message_header_t header;
    if (capwap_build_header(&header, CAPWAP_MSG_RESET_REQUEST, 0, session_id) < 0) {
        return -1;
    }

    int msg_len = sizeof(capwap_message_header_t);
    header.length = htons(msg_len - sizeof(capwap_message_header_t));
    memcpy(buf, &header, sizeof(header));

    *len = msg_len;
    return 0;
}

/* Build CAPWAP Statistics Request */
int capwap_build_statistics_request(char *buf, int *len, uint32_t session_id, uint8_t radio_id)
{
    if (!buf || !len) {
        return -1;
    }

    capwap_message_header_t header;
    if (capwap_build_header(&header, CAPWAP_MSG_STATISTICS_REQUEST, 0, session_id) < 0) {
        return -1;
    }

    /* Calculate message length */
    int msg_len = sizeof(capwap_message_header_t);

    /* Add Radio Info element */
    capwap_element_header_t elem_hdr;
    elem_hdr.type = htons(CAPWAP_ELEMENT_RADIO_INFO);
    elem_hdr.length = htons(1);

    memcpy(buf + msg_len, &elem_hdr, sizeof(elem_hdr));
    msg_len += sizeof(elem_hdr);
    memcpy(buf + msg_len, &radio_id, 1);
    msg_len += 1;

    /* Update header length */
    header.length = htons(msg_len - sizeof(capwap_message_header_t));
    memcpy(buf, &header, sizeof(header));

    *len = msg_len;
    return 0;
}

/* Parse CAPWAP message header */
int capwap_parse_header(const char *buf, int len, capwap_message_header_t *header)
{
    if (!buf || !header || len < sizeof(capwap_message_header_t)) {
        return -1;
    }

    memcpy(header, buf, sizeof(capwap_message_header_t));
    header->length = ntohs(header->length);
    header->fragment_id = ntohs(header->fragment_id);
    header->session_id = ntohl(header->session_id);

    return 0;
}

/* Parse CAPWAP message elements */
int capwap_parse_elements(const char *buf, int len, int (*element_handler)(uint16_t type, const char *data, int data_len, void *arg), void *arg)
{
    if (!buf || !element_handler) {
        return -1;
    }

    int pos = 0;
    while (pos < len) {
        if (pos + sizeof(capwap_element_header_t) > len) {
            sys_warn("capwap_parse_elements: truncated element header\n");
            return -1;
        }

        capwap_element_header_t elem_hdr;
        memcpy(&elem_hdr, buf + pos, sizeof(elem_hdr));
        uint16_t elem_type = ntohs(elem_hdr.type);
        uint16_t elem_len = ntohs(elem_hdr.length);

        pos += sizeof(capwap_element_header_t);

        if (pos + elem_len > len) {
            sys_warn("capwap_parse_elements: truncated element data\n");
            return -1;
        }

        if (element_handler(elem_type, buf + pos, elem_len, arg) < 0) {
            sys_warn("capwap_parse_elements: element handler failed\n");
        }

        pos += elem_len;
    }

    return 0;
}

/* Parse complete CAPWAP message */
int capwap_parse_message(const char *buf, int len, capwap_parsed_message_t *parsed_msg)
{
    if (!buf || !parsed_msg || len < sizeof(capwap_message_header_t)) {
        return -1;
    }

    capwap_message_header_t header;
    if (capwap_parse_header(buf, len, &header) < 0) {
        return -1;
    }

    /* Check message length */
    int total_len = sizeof(capwap_message_header_t) + header.length;
    if (total_len > len) {
        sys_warn("capwap_parse_message: message truncated\n");
        return -1;
    }

    /* Fill parsed message structure */
    parsed_msg->type = header.type;
    parsed_msg->session_id = header.session_id;
    parsed_msg->fragment_id = header.fragment_id;
    parsed_msg->payload_len = header.length;

    if (header.length > 0) {
        parsed_msg->payload = (uint8_t *)malloc(header.length);
        if (!parsed_msg->payload) {
            sys_err("capwap_parse_message: malloc failed\n");
            return -1;
        }
        memcpy(parsed_msg->payload, buf + sizeof(capwap_message_header_t), header.length);
    } else {
        parsed_msg->payload = NULL;
    }

    return 0;
}

/* Free parsed message */
void capwap_free_parsed_message(capwap_parsed_message_t *parsed_msg)
{
    if (parsed_msg) {
        if (parsed_msg->payload) {
            free(parsed_msg->payload);
            parsed_msg->payload = NULL;
        }
        parsed_msg->payload_len = 0;
    }
}

/* Get message type string */
const char *capwap_get_message_type_string(uint8_t type)
{
    switch (type) {
        case CAPWAP_MSG_DISCOVERY_REQUEST:
            return "DISCOVERY_REQUEST";
        case CAPWAP_MSG_DISCOVERY_RESPONSE:
            return "DISCOVERY_RESPONSE";
        case CAPWAP_MSG_JOIN_REQUEST:
            return "JOIN_REQUEST";
        case CAPWAP_MSG_JOIN_RESPONSE:
            return "JOIN_RESPONSE";
        case CAPWAP_MSG_CONFIGURE_REQUEST:
            return "CONFIGURE_REQUEST";
        case CAPWAP_MSG_CONFIGURE_RESPONSE:
            return "CONFIGURE_RESPONSE";
        case CAPWAP_MSG_ECHO_REQUEST:
            return "ECHO_REQUEST";
        case CAPWAP_MSG_ECHO_RESPONSE:
            return "ECHO_RESPONSE";
        case CAPWAP_MSG_RESET_REQUEST:
            return "RESET_REQUEST";
        case CAPWAP_MSG_RESET_RESPONSE:
            return "RESET_RESPONSE";
        case CAPWAP_MSG_DATA:
            return "DATA";
        case CAPWAP_MSG_WTP_EVENT_REQUEST:
            return "WTP_EVENT_REQUEST";
        case CAPWAP_MSG_IMAGE_DATA_REQUEST:
            return "IMAGE_DATA_REQUEST";
        case CAPWAP_MSG_IMAGE_DATA_RESPONSE:
            return "IMAGE_DATA_RESPONSE";
        case CAPWAP_MSG_STATISTICS_REQUEST:
            return "STATISTICS_REQUEST";
        case CAPWAP_MSG_STATISTICS_RESPONSE:
            return "STATISTICS_RESPONSE";
        default:
            return "UNKNOWN";
    }
}

/* Message mapping: existing message to CAPWAP */
int msg_map_to_capwap(int msg_type, char *data, int len, char *capwap_buf, int *capwap_len, uint32_t session_id)
{
    switch (msg_type) {
    case MSG_AC_BRD:
        return capwap_build_discovery_request(capwap_buf, capwap_len, "AC-Controller", NULL);
    case MSG_AP_REG:
        {
            struct msg_ap_reg_t *reg_msg = (struct msg_ap_reg_t *)data;
            return capwap_build_join_request(capwap_buf, capwap_len, "AP-Client", reg_msg->header.mac, session_id);
        }
    case MSG_HEARTBEAT:
        return capwap_build_echo_request(capwap_buf, capwap_len, session_id);
    case MSG_AP_STATUS:
        return capwap_build_wtp_event(capwap_buf, capwap_len, 0, data, len, session_id);
    default:
        sys_warn("msg_map_to_capwap: unknown message type %d\n", msg_type);
        return -1;
    }
}

/* Message mapping: CAPWAP to existing message */
int msg_map_from_capwap(const char *capwap_buf, int capwap_len, int *msg_type, char *data, int *len)
{
    capwap_message_header_t header;
    if (capwap_parse_header(capwap_buf, capwap_len, &header) < 0) {
        return -1;
    }

    switch (header.type) {
    case CAPWAP_MSG_DISCOVERY_RESPONSE:
        *msg_type = MSG_AC_REG_RESP;
        break;
    case CAPWAP_MSG_JOIN_RESPONSE:
        *msg_type = MSG_AC_REG_RESP;
        break;
    case CAPWAP_MSG_ECHO_RESPONSE:
        *msg_type = MSG_HEARTBEAT;
        break;
    case CAPWAP_MSG_CONFIGURE_RESPONSE:
        *msg_type = MSG_AC_CMD;
        break;
    case CAPWAP_MSG_RESET_RESPONSE:
        *msg_type = MSG_AC_CMD;
        break;
    case CAPWAP_MSG_STATISTICS_RESPONSE:
        *msg_type = MSG_AC_CMD;
        break;
    default:
        sys_warn("msg_map_from_capwap: unknown CAPWAP message type %d\n", header.type);
        return -1;
    }

    /* Parse CAPWAP message and convert to existing message format */
    capwap_parsed_message_t parsed_msg;
    if (capwap_parse_message(capwap_buf, capwap_len, &parsed_msg) >= 0) {
        /* Extract relevant data from CAPWAP message */
        if (parsed_msg.payload_len > 0 && data && len) {
            int copy_len = parsed_msg.payload_len;
            if (copy_len > 1024) {  /* Max buffer size */
                copy_len = 1024;
            }
            memcpy(data, parsed_msg.payload, copy_len);
            *len = copy_len;
        }
        capwap_free_parsed_message(&parsed_msg);
    }

    return 0;
}

/* CAPWAP Image Data element */
#define CAPWAP_ELEMENT_IMAGE_DATA  0x01
#define CAPWAP_ELEMENT_IMAGE_ID    0x02
#define CAPWAP_ELEMENT_IMAGE_INFO  0x03

/* Image Data transfer context */
typedef struct capwap_image_ctx_t {
    uint32_t image_id;
    uint32_t total_size;
    uint32_t received_size;
    char *data;
    int data_allocated;
} capwap_image_ctx_t;

static capwap_image_ctx_t g_image_ctx;

/* Build CAPWAP Image Data Request */
int capwap_build_image_data_request(char *buf, int *len, uint32_t session_id, uint32_t image_id,
                                     const char *image_data, int data_len, int offset)
{
    if (!buf || !len) {
        return -1;
    }

    capwap_message_header_t header;
    if (capwap_build_header(&header, CAPWAP_MSG_IMAGE_DATA_REQUEST, 0, session_id) < 0) {
        return -1;
    }

    int msg_len = sizeof(capwap_message_header_t);

    /* Add Image ID element */
    capwap_element_header_t elem_hdr;
    elem_hdr.type = htons(CAPWAP_ELEMENT_IMAGE_ID);
    elem_hdr.length = htons(4);

    memcpy(buf + msg_len, &elem_hdr, sizeof(elem_hdr));
    msg_len += sizeof(elem_hdr);
    uint32_t img_id = htonl(image_id);
    memcpy(buf + msg_len, &img_id, 4);
    msg_len += 4;

    /* Add Image Data element */
    if (image_data && data_len > 0) {
        elem_hdr.type = htons(CAPWAP_ELEMENT_IMAGE_DATA);
        elem_hdr.length = htons(data_len);

        memcpy(buf + msg_len, &elem_hdr, sizeof(elem_hdr));
        msg_len += sizeof(elem_hdr);
        memcpy(buf + msg_len, image_data, data_len);
        msg_len += data_len;
    }

    /* Update header length */
    header.length = htons(msg_len - sizeof(capwap_message_header_t));
    memcpy(buf, &header, sizeof(header));

    *len = msg_len;
    return 0;
}

/* Build CAPWAP Image Data Response */
int capwap_build_image_data_response(char *buf, int *len, uint32_t session_id, uint32_t image_id,
                                      int status, int offset, int total_size)
{
    if (!buf || !len) {
        return -1;
    }

    capwap_message_header_t header;
    if (capwap_build_header(&header, CAPWAP_MSG_IMAGE_DATA_RESPONSE, 0, session_id) < 0) {
        return -1;
    }

    int msg_len = sizeof(capwap_message_header_t);

    /* Add Image ID element */
    capwap_element_header_t elem_hdr;
    elem_hdr.type = htons(CAPWAP_ELEMENT_IMAGE_ID);
    elem_hdr.length = htons(4);

    memcpy(buf + msg_len, &elem_hdr, sizeof(elem_hdr));
    msg_len += sizeof(elem_hdr);
    uint32_t img_id = htonl(image_id);
    memcpy(buf + msg_len, &img_id, 4);
    msg_len += 4;

    /* Add Image Info element (status + offset + total) */
    elem_hdr.type = htons(CAPWAP_ELEMENT_IMAGE_INFO);
    elem_hdr.length = htons(12);

    memcpy(buf + msg_len, &elem_hdr, sizeof(elem_hdr));
    msg_len += sizeof(elem_hdr);

    uint32_t info[3];
    info[0] = htonl(status);
    info[1] = htonl(offset);
    info[2] = htonl(total_size);
    memcpy(buf + msg_len, info, sizeof(info));
    msg_len += sizeof(info);

    /* Update header length */
    header.length = htons(msg_len - sizeof(capwap_message_header_t));
    memcpy(buf, &header, sizeof(header));

    *len = msg_len;
    return 0;
}

/* Initialize image transfer */
void capwap_image_transfer_init(uint32_t image_id, uint32_t total_size)
{
    memset(&g_image_ctx, 0, sizeof(g_image_ctx));
    g_image_ctx.image_id = image_id;
    g_image_ctx.total_size = total_size;
    g_image_ctx.data = (char *)malloc(total_size);
    if (g_image_ctx.data) {
        g_image_ctx.data_allocated = total_size;
    }
    sys_info("Image transfer initialized: id=%u, size=%u\n", image_id, total_size);
}

/* Add image data chunk */
int capwap_image_add_chunk(uint32_t offset, const char *data, int data_len)
{
    if (!data || data_len <= 0) {
        return -1;
    }

    if (!g_image_ctx.data || offset + data_len > g_image_ctx.total_size) {
        int new_size = offset + data_len;
        char *new_data = (char *)realloc(g_image_ctx.data, new_size);
        if (!new_data) {
            sys_err("capwap_image_add_chunk: realloc failed\n");
            return -1;
        }
        g_image_ctx.data = new_data;
        g_image_ctx.data_allocated = new_size;
    }

    memcpy(g_image_ctx.data + offset, data, data_len);
    g_image_ctx.received_size += data_len;

    sys_debug("Image chunk added: offset=%u, len=%d, total=%u/%u\n",
              offset, data_len, g_image_ctx.received_size, g_image_ctx.total_size);

    return 0;
}

/* Get received image data */
char *capwap_image_get_data(uint32_t *total_len)
{
    if (total_len) {
        *total_len = g_image_ctx.received_size;
    }
    return g_image_ctx.data;
}

/* Check if image transfer is complete */
int capwap_image_is_complete(void)
{
    return (g_image_ctx.received_size >= g_image_ctx.total_size);
}

/* Clear image transfer context */
void capwap_image_transfer_clear(void)
{
    if (g_image_ctx.data) {
        free(g_image_ctx.data);
    }
    memset(&g_image_ctx, 0, sizeof(g_image_ctx));
}