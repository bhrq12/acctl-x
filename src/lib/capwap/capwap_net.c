/*
 * ============================================================================
 *
 *       Filename:  capwap_net.c
 *
 *    Description:  CAPWAP network layer implementation
 *                  UDP + DTLS for CAPWAP control and data channels
 *
 *        Version:  1.0
 *        Created:  2026-04-26
 *       Revision:  initial version
 *       Compiler:  gcc
 *
 * ============================================================================
 */
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>
#include <fcntl.h>

#include "log.h"
#include "msg.h"
#include "capwap/capwap.h"

/* CAPWAP network context */
typedef struct capwap_net_ctx_t {
    capwap_conn_t conn;
    int mode;  /* 0: AC, 1: AP */
    int (*msg_handler)(char *data, int len, void *arg);
    void *msg_handler_arg;
    pthread_t recv_thread;
    int running;
} capwap_net_ctx_t;

/* Global CAPWAP network context */
static capwap_net_ctx_t g_capwap_ctx;

/* Initialize CAPWAP network layer */
int capwap_net_init(int mode, const char *ac_ip, int ac_port)
{
    sys_info("Initializing CAPWAP network layer (mode: %d)\n", mode);
    
    /* Initialize CAPWAP connection */
    if (capwap_init(&g_capwap_ctx.conn) < 0) {
        sys_err("capwap_init failed\n");
        return -1;
    }
    
    g_capwap_ctx.mode = mode;
    g_capwap_ctx.running = 0;
    g_capwap_ctx.msg_handler = NULL;
    g_capwap_ctx.msg_handler_arg = NULL;
    
    if (mode == 1) {  /* AP mode */
        if (ac_ip && ac_port > 0) {
            if (capwap_connect(&g_capwap_ctx.conn, ac_ip, ac_port) < 0) {
                sys_err("capwap_connect failed\n");
                return -1;
            }
        }
    } else {  /* AC mode */
        /* TODO: Initialize AC mode */
        sys_info("CAPWAP AC mode initialized\n");
    }
    
    return 0;
}

/* Set message handler */
void capwap_net_set_msg_handler(int (*handler)(char *data, int len, void *arg), void *arg)
{
    g_capwap_ctx.msg_handler = handler;
    g_capwap_ctx.msg_handler_arg = arg;
}

/* Send CAPWAP message */
int capwap_net_send(int msg_type, char *data, int len)
{
    switch (msg_type) {
    case MSG_AC_BRD:
        return capwap_send_discovery(&g_capwap_ctx.conn);
    case MSG_AP_REG:
        return capwap_send_join(&g_capwap_ctx.conn);
    case MSG_HEARTBEAT:
        return capwap_send_echo(&g_capwap_ctx.conn);
    case MSG_AP_STATUS:
        return capwap_send_event(&g_capwap_ctx.conn, 0, data);
    default:
        sys_warn("capwap_net_send: unknown message type %d\n", msg_type);
        return -1;
    }
}

/* Receive thread function */
static void *capwap_net_recv_thread(void *arg)
{
    char buf[4096];
    struct sockaddr_in from;
    socklen_t from_len = sizeof(from);
    
    sys_info("CAPWAP receive thread started\n");
    
    while (g_capwap_ctx.running) {
        int recv_len = recvfrom(g_capwap_ctx.conn.ctrl_sock, buf, sizeof(buf), 0,
                               (struct sockaddr *)&from, &from_len);
        
        if (recv_len > 0) {
            sys_debug("Received CAPWAP message (len: %d)\n", recv_len);
            
            if (g_capwap_ctx.msg_handler) {
                g_capwap_ctx.msg_handler(buf, recv_len, g_capwap_ctx.msg_handler_arg);
            }
        } else if (recv_len < 0) {
            if (errno != EAGAIN && errno != EINTR) {
                sys_err("capwap_net_recv_thread: recvfrom failed: %s\n", strerror(errno));
                break;
            }
        }
        
        usleep(10000);  /* 10ms delay */
    }
    
    sys_info("CAPWAP receive thread stopped\n");
    return NULL;
}

/* Start CAPWAP network layer */
int capwap_net_start(void)
{
    if (g_capwap_ctx.running) {
        return 0;
    }
    
    g_capwap_ctx.running = 1;
    
    if (pthread_create(&g_capwap_ctx.recv_thread, NULL, capwap_net_recv_thread, NULL) < 0) {
        sys_err("pthread_create failed: %s\n", strerror(errno));
        g_capwap_ctx.running = 0;
        return -1;
    }
    
    sys_info("CAPWAP network layer started\n");
    return 0;
}

/* Stop CAPWAP network layer */
int capwap_net_stop(void)
{
    if (!g_capwap_ctx.running) {
        return 0;
    }
    
    g_capwap_ctx.running = 0;
    pthread_join(g_capwap_ctx.recv_thread, NULL);
    
    if (g_capwap_ctx.conn.ctrl_sock >= 0) {
        capwap_disconnect(&g_capwap_ctx.conn);
    }
    
    sys_info("CAPWAP network layer stopped\n");
    return 0;
}

/* Get CAPWAP connection state */
enum capwap_state capwap_net_get_state(void)
{
    return g_capwap_ctx.conn.state;
}

/* Set CAPWAP connection state */
void capwap_net_set_state(enum capwap_state state)
{
    g_capwap_ctx.conn.state = state;
}

/* Get AC address */
struct sockaddr_in *capwap_net_get_ac_addr(void)
{
    return &g_capwap_ctx.conn.ac_addr;
}

/* Get WTP address */
struct sockaddr_in *capwap_net_get_wtp_addr(void)
{
    return &g_capwap_ctx.conn.wtp_addr;
}