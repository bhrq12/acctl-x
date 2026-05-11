/*
 * ============================================================================
 *
 *       Filename:  capwap.c
 *
 *    Description:  CAPWAP protocol integration implementation
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
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#include "log.h"
#include "capwap/capwap.h"
#include "capwap/capwap_msg.h"

/* CAPWAP element data context for passing parsed data */
struct capwap_element_ctx {
    char ac_name[64];
    char wtp_name[64];
    uint8_t mac_address[6];
    uint8_t radio_id;
    uint8_t status;
    int has_ac_name;
    int has_wtp_name;
    int has_mac_address;
    int has_radio_info;
    int has_status;
};

/* Element handler for message parsing */
static int capwap_element_handler(uint16_t type, const char *data, int data_len, void *arg)
{
    struct capwap_element_ctx *ctx = (struct capwap_element_ctx *)arg;
    size_t dlen = (size_t)data_len;

    switch (type) {
        case CAPWAP_ELEMENT_AC_NAME:
            if (data && data_len > 0 && dlen < sizeof(ctx->ac_name)) {
                memcpy(ctx->ac_name, data, dlen);
                ctx->ac_name[dlen] = '\0';
                ctx->has_ac_name = 1;
                sys_info("CAPWAP element: AC_NAME=%s\n", ctx->ac_name);
            }
            break;

        case CAPWAP_ELEMENT_WTP_NAME:
            if (data && data_len > 0 && dlen < sizeof(ctx->wtp_name)) {
                memcpy(ctx->wtp_name, data, dlen);
                ctx->wtp_name[dlen] = '\0';
                ctx->has_wtp_name = 1;
                sys_info("CAPWAP element: WTP_NAME=%s\n", ctx->wtp_name);
            }
            break;

        case CAPWAP_ELEMENT_MAC_ADDRESS:
            if (data && data_len >= 6) {
                memcpy(ctx->mac_address, data, 6);
                ctx->has_mac_address = 1;
                sys_info("CAPWAP element: MAC_ADDRESS=%02x:%02x:%02x:%02x:%02x:%02x\n",
                         ctx->mac_address[0], ctx->mac_address[1], ctx->mac_address[2],
                         ctx->mac_address[3], ctx->mac_address[4], ctx->mac_address[5]);
            }
            break;

        case CAPWAP_ELEMENT_RADIO_INFO:
            if (data && data_len >= 1) {
                ctx->radio_id = data[0];
                ctx->has_radio_info = 1;
                sys_info("CAPWAP element: RADIO_INFO=radio_id=%d\n", ctx->radio_id);
            }
            break;

        case CAPWAP_ELEMENT_STATUS:
            if (data && data_len >= 1) {
                ctx->status = data[0];
                ctx->has_status = 1;
                sys_info("CAPWAP element: STATUS=%d\n", ctx->status);
            }
            break;

        case CAPWAP_ELEMENT_CAPABILITY:
            if (data && data_len >= 2) {
                uint16_t caps = (data[0] << 8) | data[1];
                sys_info("CAPWAP element: CAPABILITY=0x%04x\n", caps);
            }
            break;

        case CAPWAP_ELEMENT_TIMERS:
            if (data && data_len >= 2) {
                uint16_t timers = (data[0] << 8) | data[1];
                sys_info("CAPWAP element: TIMERS=0x%04x\n", timers);
            }
            break;

        case CAPWAP_ELEMENT_DATA_TRANSFER_MODE:
            if (data && data_len >= 1) {
                sys_info("CAPWAP element: DATA_TRANSFER_MODE=%d\n", data[0]);
            }
            break;

        case CAPWAP_ELEMENT_CONTROL_IP_ADDRESS:
            if (data && data_len >= 6) {
                sys_info("CAPWAP element: CONTROL_IP_ADDRESS\n");
            }
            break;

        case CAPWAP_ELEMENT_WTP_BOARD_DATA:
            sys_debug("CAPWAP element: WTP_BOARD_DATA (len=%d)\n", data_len);
            break;

        case CAPWAP_ELEMENT_WTP_DESCRIPTOR:
            sys_debug("CAPWAP element: WTP_DESCRIPTOR (len=%d)\n", data_len);
            break;

        case CAPWAP_ELEMENT_LOCATION_DATA:
            sys_debug("CAPWAP element: LOCATION_DATA (len=%d)\n", data_len);
            break;

        case CAPWAP_ELEMENT_STATISTICS:
            sys_debug("CAPWAP element: STATISTICS (len=%d)\n", data_len);
            break;

        case CAPWAP_ELEMENT_REBOOT_STATISTICS:
            sys_debug("CAPWAP element: REBOOT_STATISTICS (len=%d)\n", data_len);
            break;

        case CAPWAP_ELEMENT_EVENT:
            sys_debug("CAPWAP element: EVENT (len=%d)\n", data_len);
            break;

        case CAPWAP_ELEMENT_AC_ID:
            if (data && data_len >= 4) {
                uint32_t ac_id = ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
                                 ((uint32_t)data[2] << 8) | data[3];
                sys_info("CAPWAP element: AC_ID=%u\n", ac_id);
            }
            break;

        case CAPWAP_ELEMENT_WTP_ID:
            sys_debug("CAPWAP element: WTP_ID (len=%d)\n", data_len);
            break;

        case CAPWAP_ELEMENT_VENDOR_SPECIFIC:
            sys_debug("CAPWAP element: VENDOR_SPECIFIC (len=%d)\n", data_len);
            break;

        default:
            sys_warn("CAPWAP element: unknown type=%d, len=%d\n", type, data_len);
            break;
    }

    return 0;
}

/* Handle Discovery Response */
static int capwap_handle_discovery_response(capwap_conn_t *conn, capwap_parsed_message_t *parsed_msg)
{
    sys_info("Received Discovery Response (session: %u)\n", parsed_msg->session_id);
    
    /* Extract session ID from response */
    if (parsed_msg->session_id > 0) {
        conn->session_id = parsed_msg->session_id;
        sys_info("Assigned session ID: %u\n", conn->session_id);
    }
    
    /* Transition to JOIN state */
    conn->state = CAPWAP_STATE_JOIN;
    
    /* Send Join Request */
    return capwap_send_join(conn);
}

/* Handle Join Response */
static int capwap_handle_join_response(capwap_conn_t *conn, capwap_parsed_message_t *parsed_msg)
{
    sys_info("Received Join Response (session: %u)\n", parsed_msg->session_id);
    
    /* Transition to CONFIGURE state */
    conn->state = CAPWAP_STATE_CONFIGURE;
    
    /* Send Configure Request */
    return capwap_send_configure(conn);
}

/* Handle Configure Response */
static int capwap_handle_configure_response(capwap_conn_t *conn, capwap_parsed_message_t *parsed_msg)
{
    sys_info("Received Configure Response (session: %u)\n", parsed_msg->session_id);
    
    /* Transition to RUN state */
    conn->state = CAPWAP_STATE_RUN;
    sys_info("CAPWAP session established, entering RUN state\n");
    
    return 0;
}

/* Handle Echo Response */
static int capwap_handle_echo_response(capwap_conn_t *conn, capwap_parsed_message_t *parsed_msg)
{
    sys_debug("Received Echo Response (session: %u)\n", parsed_msg->session_id);
    /* Echo response received, connection is active */
    return 0;
}

/* Handle Reset Response */
static int capwap_handle_reset_response(capwap_conn_t *conn, capwap_parsed_message_t *parsed_msg)
{
    sys_info("Received Reset Response (session: %u)\n", parsed_msg->session_id);
    
    /* Transition back to IDLE state */
    conn->state = CAPWAP_STATE_IDLE;
    sys_info("CAPWAP session reset\n");
    
    return 0;
}

/* Handle Statistics Response */
static int capwap_handle_statistics_response(capwap_conn_t *conn, capwap_parsed_message_t *parsed_msg)
{
    sys_debug("Received Statistics Response (session: %u)\n", parsed_msg->session_id);
    /* Process statistics data */
    return 0;
}

/* Initialize CAPWAP connection */
int capwap_init(capwap_conn_t *conn)
{
    if (!conn) {
        sys_err("capwap_init: conn is NULL\n");
        return -1;
    }

    memset(conn, 0, sizeof(capwap_conn_t));
    conn->state = CAPWAP_STATE_IDLE;
    conn->sequence = 0;
    conn->session_id = 0;
    conn->ctrl_sock = -1;
    conn->data_sock = -1;
    conn->dtls_session = NULL;

    return 0;
}

/* Create UDP socket for CAPWAP */
static int capwap_create_socket(void)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        sys_err("capwap_create_socket: %s\n", strerror(errno));
        return -1;
    }

    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        sys_warn("capwap_create_socket: setsockopt SO_REUSEADDR: %s\n", strerror(errno));
    }

    return sock;
}

/* Connect to AC */
int capwap_connect(capwap_conn_t *conn, const char *ac_ip, int ac_port)
{
    if (!conn || !ac_ip) {
        sys_err("capwap_connect: invalid parameters\n");
        return -1;
    }

    /* Create control channel socket */
    conn->ctrl_sock = capwap_create_socket();
    if (conn->ctrl_sock < 0) {
        return -1;
    }

    /* Create data channel socket */
    conn->data_sock = capwap_create_socket();
    if (conn->data_sock < 0) {
        close(conn->ctrl_sock);
        conn->ctrl_sock = -1;
        return -1;
    }

    /* Set AC address */
    memset(&conn->ac_addr, 0, sizeof(conn->ac_addr));
    conn->ac_addr.sin_family = AF_INET;
    conn->ac_addr.sin_port = htons(ac_port);
    if (inet_pton(AF_INET, ac_ip, &conn->ac_addr.sin_addr) <= 0) {
        sys_err("capwap_connect: invalid AC IP address\n");
        capwap_disconnect(conn);
        return -1;
    }

    conn->state = CAPWAP_STATE_DISCOVERY;
    sys_info("CAPWAP connected to AC %s:%d\n", ac_ip, ac_port);
    return 0;
}

/* Disconnect from AC */
int capwap_disconnect(capwap_conn_t *conn)
{
    if (!conn) {
        return -1;
    }

    if (conn->ctrl_sock >= 0) {
        close(conn->ctrl_sock);
        conn->ctrl_sock = -1;
    }

    if (conn->data_sock >= 0) {
        close(conn->data_sock);
        conn->data_sock = -1;
    }

    if (conn->dtls_session) {
        capwap_dtls_cleanup(conn);
    }

    conn->state = CAPWAP_STATE_IDLE;
    sys_info("CAPWAP disconnected\n");
    return 0;
}

/* Send Discovery Request */
int capwap_send_discovery(capwap_conn_t *conn)
{
    if (!conn || conn->ctrl_sock < 0) {
        return -1;
    }

    char buf[1024];
    int len = 0;

    if (capwap_build_discovery_request(buf, &len, "AC-Controller", NULL) < 0) {
        sys_err("capwap_send_discovery: failed to build message\n");
        return -1;
    }

    sys_debug("Sending CAPWAP Discovery Request (len: %d)\n", len);

    int sent = sendto(conn->ctrl_sock, buf, len, 0,
                    (struct sockaddr *)&conn->ac_addr, sizeof(conn->ac_addr));
    if (sent < 0) {
        sys_err("capwap_send_discovery: %s\n", strerror(errno));
        return -1;
    }

    conn->state = CAPWAP_STATE_DISCOVERY;
    conn->sequence++;
    return 0;
}

/* Send Join Request */
int capwap_send_join(capwap_conn_t *conn)
{
    if (!conn || conn->ctrl_sock < 0) {
        return -1;
    }

    char buf[1024];
    int len = 0;

    uint8_t dummy_mac[6] = {0};
    if (capwap_build_join_request(buf, &len, "AC-Controller", dummy_mac, conn->session_id) < 0) {
        sys_err("capwap_send_join: failed to build message\n");
        return -1;
    }

    sys_debug("Sending CAPWAP Join Request (len: %d, session: %u)\n", len, conn->session_id);

    int sent = sendto(conn->ctrl_sock, buf, len, 0,
                    (struct sockaddr *)&conn->ac_addr, sizeof(conn->ac_addr));
    if (sent < 0) {
        sys_err("capwap_send_join: %s\n", strerror(errno));
        return -1;
    }

    conn->state = CAPWAP_STATE_JOIN;
    conn->sequence++;
    return 0;
}

/* Send Configure Request */
int capwap_send_configure(capwap_conn_t *conn)
{
    if (!conn || conn->ctrl_sock < 0) {
        return -1;
    }

    char buf[1024];
    int len = 0;

    /* Sample configuration data */
    const char *config_data = "{\"radio\":{\"enabled\":true,\"channel\":6}}";
    int config_len = strlen(config_data);

    if (capwap_build_configure_request(buf, &len, conn->session_id, 0, config_data, config_len) < 0) {
        sys_err("capwap_send_configure: failed to build message\n");
        return -1;
    }

    sys_debug("Sending CAPWAP Configure Request (len: %d, session: %u)\n", len, conn->session_id);

    int sent = sendto(conn->ctrl_sock, buf, len, 0,
                    (struct sockaddr *)&conn->ac_addr, sizeof(conn->ac_addr));
    if (sent < 0) {
        sys_err("capwap_send_configure: %s\n", strerror(errno));
        return -1;
    }

    conn->sequence++;
    return 0;
}

/* Send Echo Request */
int capwap_send_echo(capwap_conn_t *conn)
{
    if (!conn || conn->ctrl_sock < 0) {
        return -1;
    }

    char buf[1024];
    int len = 0;

    if (capwap_build_echo_request(buf, &len, conn->session_id) < 0) {
        sys_err("capwap_send_echo: failed to build message\n");
        return -1;
    }

    sys_debug("Sending CAPWAP Echo Request (len: %d, session: %u)\n", len, conn->session_id);

    int sent = sendto(conn->ctrl_sock, buf, len, 0,
                    (struct sockaddr *)&conn->ac_addr, sizeof(conn->ac_addr));
    if (sent < 0) {
        sys_err("capwap_send_echo: %s\n", strerror(errno));
        return -1;
    }

    conn->sequence++;
    return 0;
}

/* Send Reset Request */
int capwap_send_reset(capwap_conn_t *conn)
{
    if (!conn || conn->ctrl_sock < 0) {
        return -1;
    }

    char buf[1024];
    int len = 0;

    if (capwap_build_reset_request(buf, &len, conn->session_id) < 0) {
        sys_err("capwap_send_reset: failed to build message\n");
        return -1;
    }

    sys_debug("Sending CAPWAP Reset Request (len: %d, session: %u)\n", len, conn->session_id);

    int sent = sendto(conn->ctrl_sock, buf, len, 0,
                    (struct sockaddr *)&conn->ac_addr, sizeof(conn->ac_addr));
    if (sent < 0) {
        sys_err("capwap_send_reset: %s\n", strerror(errno));
        return -1;
    }

    conn->sequence++;
    return 0;
}

/* Send Statistics Request */
int capwap_send_statistics(capwap_conn_t *conn, uint8_t radio_id)
{
    if (!conn || conn->ctrl_sock < 0) {
        return -1;
    }

    char buf[1024];
    int len = 0;

    if (capwap_build_statistics_request(buf, &len, conn->session_id, radio_id) < 0) {
        sys_err("capwap_send_statistics: failed to build message\n");
        return -1;
    }

    sys_debug("Sending CAPWAP Statistics Request (len: %d, session: %u, radio: %d)\n", len, conn->session_id, radio_id);

    int sent = sendto(conn->ctrl_sock, buf, len, 0,
                    (struct sockaddr *)&conn->ac_addr, sizeof(conn->ac_addr));
    if (sent < 0) {
        sys_err("capwap_send_statistics: %s\n", strerror(errno));
        return -1;
    }

    conn->sequence++;
    return 0;
}

/* Send WTP Event */
int capwap_send_event(capwap_conn_t *conn, uint16_t event_type, void *event_data)
{
    if (!conn || conn->ctrl_sock < 0) {
        return -1;
    }

    char buf[1024];
    int len = 0;
    int event_data_len = 0;

    if (capwap_build_wtp_event(buf, &len, event_type, event_data, event_data_len, conn->session_id) < 0) {
        sys_err("capwap_send_event: failed to build message\n");
        return -1;
    }

    sys_debug("Sending CAPWAP WTP Event Request (type: %d, len: %d, session: %u)\n",
              event_type, len, conn->session_id);

    int sent = sendto(conn->ctrl_sock, buf, len, 0,
                    (struct sockaddr *)&conn->ac_addr, sizeof(conn->ac_addr));
    if (sent < 0) {
        sys_err("capwap_send_event: %s\n", strerror(errno));
        return -1;
    }

    conn->sequence++;
    return 0;
}

/* Process incoming CAPWAP message */
int capwap_process_message(capwap_conn_t *conn, char *data, int len)
{
    if (!conn || !data || len <= 0) {
        return -1;
    }

    /* Parse CAPWAP message */
    capwap_parsed_message_t parsed_msg;
    if (capwap_parse_message(data, len, &parsed_msg) < 0) {
        sys_err("capwap_process_message: failed to parse message\n");
        return -1;
    }

    sys_debug("Processing CAPWAP message: %s (session: %u, len: %d)\n",
              capwap_get_message_type_string(parsed_msg.type),
              parsed_msg.session_id, parsed_msg.payload_len);

    /* Process message based on type */
    int ret = 0;
    switch (parsed_msg.type) {
        case CAPWAP_MSG_DISCOVERY_RESPONSE:
            ret = capwap_handle_discovery_response(conn, &parsed_msg);
            break;
        case CAPWAP_MSG_JOIN_RESPONSE:
            ret = capwap_handle_join_response(conn, &parsed_msg);
            break;
        case CAPWAP_MSG_CONFIGURE_RESPONSE:
            ret = capwap_handle_configure_response(conn, &parsed_msg);
            break;
        case CAPWAP_MSG_ECHO_RESPONSE:
            ret = capwap_handle_echo_response(conn, &parsed_msg);
            break;
        case CAPWAP_MSG_RESET_RESPONSE:
            ret = capwap_handle_reset_response(conn, &parsed_msg);
            break;
        case CAPWAP_MSG_STATISTICS_RESPONSE:
            ret = capwap_handle_statistics_response(conn, &parsed_msg);
            break;
        default:
            sys_warn("Unknown CAPWAP message type: %d\n", parsed_msg.type);
            break;
    }

    /* Parse message elements if payload exists */
    if (parsed_msg.payload_len > 0) {
        struct capwap_element_ctx elem_ctx;
        memset(&elem_ctx, 0, sizeof(elem_ctx));
        capwap_parse_elements((const char *)parsed_msg.payload, parsed_msg.payload_len,
                            capwap_element_handler, &elem_ctx);
    }

    /* Free parsed message */
    capwap_free_parsed_message(&parsed_msg);
    return ret;
}

/* Data channel context for keepalive */
typedef struct capwap_keepalive_ctx_t {
    capwap_conn_t *conn;
    pthread_t thread;
    volatile int running;
    int interval_sec;
} capwap_keepalive_ctx_t;

static capwap_keepalive_ctx_t g_keepalive_ctx;

/* Keepalive thread function */
static void *capwap_keepalive_thread(void *arg)
{
    capwap_keepalive_ctx_t *ctx = (capwap_keepalive_ctx_t *)arg;

    sys_info("CAPWAP keepalive thread started (interval: %d seconds)\n", ctx->interval_sec);

    while (ctx->running) {
        for (int i = 0; i < ctx->interval_sec && ctx->running; i++) {
            usleep(1000000);
        }

        if (!ctx->running) {
            break;
        }

        if (ctx->conn && ctx->conn->state == CAPWAP_STATE_RUN) {
            if (capwap_send_echo(ctx->conn) < 0) {
                sys_warn("CAPWAP keepalive: failed to send echo request\n");
            } else {
                sys_debug("CAPWAP keepalive: echo request sent\n");
            }
        }
    }

    sys_info("CAPWAP keepalive thread stopped\n");
    return NULL;
}

/* Setup data channel */
int capwap_setup_data_channel(capwap_conn_t *conn)
{
    if (!conn || conn->data_sock < 0) {
        return -1;
    }

    sys_info("Setting up CAPWAP data channel\n");

    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(CAPWAP_PORT_DATA);

    if (bind(conn->data_sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        sys_err("capwap_setup_data_channel: bind failed: %s\n", strerror(errno));
        return -1;
    }

    int opt = 1;
    if (setsockopt(conn->data_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        sys_warn("capwap_setup_data_channel: setsockopt SO_REUSEADDR: %s\n", strerror(errno));
    }

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    if (setsockopt(conn->data_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        sys_warn("capwap_setup_data_channel: setsockopt SO_RCVTIMEO: %s\n", strerror(errno));
    }

    sys_info("CAPWAP data channel setup complete (port: %d)\n", CAPWAP_PORT_DATA);
    return 0;
}

/* Start keepalive mechanism */
void capwap_start_keepalive(capwap_conn_t *conn)
{
    if (!conn) {
        return;
    }

    if (g_keepalive_ctx.running) {
        sys_warn("CAPWAP keepalive already running\n");
        return;
    }

    g_keepalive_ctx.conn = conn;
    g_keepalive_ctx.running = 1;
    g_keepalive_ctx.interval_sec = 30;

    if (pthread_create(&g_keepalive_ctx.thread, NULL, capwap_keepalive_thread, &g_keepalive_ctx) < 0) {
        sys_err("capwap_start_keepalive: pthread_create failed: %s\n", strerror(errno));
        g_keepalive_ctx.running = 0;
        return;
    }

    sys_info("CAPWAP keepalive started (interval: %d seconds)\n", g_keepalive_ctx.interval_sec);
}

/* Stop keepalive mechanism */
void capwap_stop_keepalive(void)
{
    if (!g_keepalive_ctx.running) {
        return;
    }

    g_keepalive_ctx.running = 0;
    pthread_join(g_keepalive_ctx.thread, NULL);
    sys_info("CAPWAP keepalive stopped\n");
}

/* Get current state */
enum capwap_state capwap_get_state(capwap_conn_t *conn)
{
    if (!conn) {
        return CAPWAP_STATE_IDLE;
    }
    return conn->state;
}

/* Message retransmission context */
typedef struct capwap_retransmit_ctx_t {
    char *buffer;
    int len;
    int retries;
    int max_retries;
    time_t last_send_time;
} capwap_retransmit_ctx_t;

#define MAX_RETRANSMIT_ENTRIES 16
#define DEFAULT_MAX_RETRIES 5
#define RETRANSMIT_TIMEOUT_SEC 5

static capwap_retransmit_ctx_t g_retransmit_table[MAX_RETRANSMIT_ENTRIES];
static int g_retransmit_count = 0;

/* Add message to retransmission table */
int capwap_add_retransmit(uint8_t msg_type, char *buf, int len)
{
    if (!buf || len <= 0 || g_retransmit_count >= MAX_RETRANSMIT_ENTRIES) {
        return -1;
    }

    capwap_retransmit_ctx_t *entry = &g_retransmit_table[g_retransmit_count];
    entry->buffer = (char *)malloc(len);
    if (!entry->buffer) {
        sys_err("capwap_add_retransmit: malloc failed\n");
        return -1;
    }

    memcpy(entry->buffer, buf, len);
    entry->len = len;
    entry->retries = 0;
    entry->max_retries = DEFAULT_MAX_RETRIES;
    entry->last_send_time = time(NULL);
    g_retransmit_count++;

    sys_debug("Added message type %d to retransmit table (count: %d)\n", msg_type, g_retransmit_count);
    return 0;
}

/* Check and retransmit pending messages */
int capwap_check_retransmit(capwap_conn_t *conn)
{
    if (!conn || conn->ctrl_sock < 0) {
        return -1;
    }

    time_t now = time(NULL);
    int retransmitted = 0;

    for (int i = 0; i < g_retransmit_count; i++) {
        capwap_retransmit_ctx_t *entry = &g_retransmit_table[i];

        if (entry->retries >= entry->max_retries) {
            sys_warn("Message retransmit exceeded max retries, dropping\n");
            free(entry->buffer);
            entry->buffer = NULL;
            continue;
        }

        if (now - entry->last_send_time >= RETRANSMIT_TIMEOUT_SEC) {
            ssize_t sent = sendto(conn->ctrl_sock, entry->buffer, entry->len, 0,
                                  (struct sockaddr *)&conn->ac_addr, sizeof(conn->ac_addr));
            if (sent > 0) {
                entry->last_send_time = now;
                entry->retries++;
                retransmitted++;
                sys_debug("Retransmitted message (retry: %d/%d)\n",
                          entry->retries, entry->max_retries);
            }
        }
    }

    return retransmitted;
}

/* Remove message from retransmission table by type */
void capwap_remove_retransmit(uint8_t msg_type)
{
    for (int i = 0; i < g_retransmit_count; i++) {
        if (g_retransmit_table[i].buffer) {
            free(g_retransmit_table[i].buffer);
        }
    }
    g_retransmit_count = 0;
    sys_debug("Removed message type %d from retransmit table\n", msg_type);
}

/* Fragment a large message into CAPWAP fragments */
int capwap_fragment_message(const char *data, int len, char **fragments, int *fragment_count, int max_frag_size)
{
    if (!data || len <= 0 || !fragments || !fragment_count) {
        return -1;
    }

    int fragment_id = (int)(time(NULL) % 65536);
    int header_size = sizeof(capwap_message_header_t);
    int payload_per_frag = max_frag_size - header_size;

    if (payload_per_frag <= 0) {
        return -1;
    }

    int num_fragments = (len + payload_per_frag - 1) / payload_per_frag;
    *fragment_count = num_fragments;

    *fragments = (char *)malloc(num_fragments * max_frag_size);
    if (!*fragments) {
        sys_err("capwap_fragment_message: malloc failed\n");
        return -1;
    }

    for (int i = 0; i < num_fragments; i++) {
        int offset = i * payload_per_frag;
        int frag_len = (i == num_fragments - 1) ? (len - offset) : payload_per_frag;
        int total_len = header_size + frag_len;

        capwap_message_header_t header;
        memset(&header, 0, sizeof(header));
        header.version = 0x02;
        header.fragment_id = htons(fragment_id);
        header.fragment_offset = i;
        header.length = htons(frag_len);

        char *frag_ptr = (*fragments) + (i * max_frag_size);
        memcpy(frag_ptr, &header, header_size);
        memcpy(frag_ptr + header_size, data + offset, frag_len);
    }

    sys_debug("Fragmented message into %d fragments (fragment_id: %d)\n",
              num_fragments, fragment_id);
    return fragment_id;
}

/* Reassemble CAPWAP fragments into complete message */
typedef struct capwap_fragment_ctx_t {
    int fragment_id;
    int fragment_count;
    int received_count;
    char *data;
    int data_len;
    int allocated;
} capwap_fragment_ctx_t;

static capwap_fragment_ctx_t g_frag_ctx;

int capwap_reassemble_message(capwap_message_header_t *header, const char *payload, int payload_len)
{
    if (!header || !payload) {
        return -1;
    }

    int fragment_id = ntohs(header->fragment_id);
    int fragment_offset = header->fragment_offset;

    if (g_frag_ctx.fragment_id != fragment_id) {
        if (g_frag_ctx.data) {
            free(g_frag_ctx.data);
        }
        memset(&g_frag_ctx, 0, sizeof(g_frag_ctx));
        g_frag_ctx.fragment_id = fragment_id;
        g_frag_ctx.data = (char *)malloc(65536);
        if (!g_frag_ctx.data) {
            return -1;
        }
        g_frag_ctx.allocated = 65536;
    }

    int required_size = (fragment_offset + 1) * payload_len;
    if (required_size > g_frag_ctx.allocated) {
        char *new_data = (char *)realloc(g_frag_ctx.data, required_size * 2);
        if (!new_data) {
            return -1;
        }
        g_frag_ctx.data = new_data;
        g_frag_ctx.allocated = required_size * 2;
    }

    memcpy(g_frag_ctx.data + (fragment_offset * payload_len), payload, payload_len);
    g_frag_ctx.received_count++;
    g_frag_ctx.data_len = (fragment_offset + 1) * payload_len;

    sys_debug("Reassembling fragment: id=%d, offset=%d, received=%d\n",
              fragment_id, fragment_offset, g_frag_ctx.received_count);

    return 0;
}

/* Get reassembled data */
char *capwap_get_reassembled_data(int *len)
{
    if (len) {
        *len = g_frag_ctx.data_len;
    }
    return g_frag_ctx.data;
}

/* Clear reassembly context */
void capwap_clear_reassembly(void)
{
    if (g_frag_ctx.data) {
        free(g_frag_ctx.data);
    }
    memset(&g_frag_ctx, 0, sizeof(g_frag_ctx));
}

/* Initialize DTLS for server */
int capwap_dtls_init_server_wrapper(capwap_conn_t *conn, const char *cert_file, const char *key_file)
{
    return capwap_dtls_init_server(conn, cert_file, key_file);
}

/* Initialize DTLS for client */
int capwap_dtls_init_client_wrapper(capwap_conn_t *conn)
{
    return capwap_dtls_init_client(conn);
}

/* Perform DTLS handshake */
int capwap_dtls_handshake_wrapper(capwap_conn_t *conn)
{
    return capwap_dtls_handshake(conn);
}

/* Encrypt data with DTLS */
int capwap_dtls_encrypt_wrapper(capwap_conn_t *conn, char *data, int len, char *encrypted, int *encrypted_len)
{
    return capwap_dtls_encrypt(conn, data, len, encrypted, encrypted_len);
}

/* Decrypt data with DTLS */
int capwap_dtls_decrypt_wrapper(capwap_conn_t *conn, char *encrypted, int len, char *data, int *data_len)
{
    return capwap_dtls_decrypt(conn, encrypted, len, data, data_len);
}

/* Generate self-signed certificate */
int capwap_dtls_generate_cert_wrapper(const char *cert_file, const char *key_file, const char *common_name)
{
    return capwap_dtls_generate_cert(cert_file, key_file, common_name);
}