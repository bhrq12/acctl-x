/*
 * =====================================================================================
 *
 *       Filename:  net.h
 *
 *    Description:  Network layer abstraction header — single canonical version.
 *                  Compiled into both acser and apctl.
 *                  All other net.h files in subdirectories must NOT exist.
 *
 *        Version:  2.0
 *        Created:  2026-04-12
 *       Revision:  unified header (no duplicates in subdirectories)
 *       Compiler:  gcc
 *
 *         Author:  jianxi sun (jianxi), ycsunjane@gmail.com
 *   Organization:  OpenWrt AC Controller Project
 *
 * =====================================================================================
 */
#ifndef __NET_H__
#define __NET_H__

#include <netinet/in.h>
#include <linux/if_ether.h>

/* Protocol selectors for net_send() */
#define MSG_PROTO_ETH     (8000)
#define MSG_PROTO_TCP     (8001)

/*
 * net_init — initialize the network layer
 *   AC: starts TCP listener + ETH broadcast thread
 *   AP: starts ETH receive + epoll event loop
 *   Returns: 0 on success, -1 on error
 */
int net_init(void);

/*
 * net_send — send a packet via the specified protocol
 *   @proto:  MSG_PROTO_ETH (Ethernet) or MSG_PROTO_TCP
 *   @sock:   TCP socket fd (for MSG_PROTO_TCP; ignored for ETH)
 *   @dmac:   destination MAC address (for MSG_PROTO_ETH)
 *   @msg:    message payload
 *   @size:   payload size in bytes
 *   Returns: bytes sent, or -1 on error
 */
int net_send(int proto, int sock, char *dmac, char *msg, int size);

/*
 * net_send_tcp — send data via TCP connection
 *   @ip:     destination IP address
 *   @port:   destination port
 *   @data:   data to send
 *   @len:    data length in bytes
 *   Returns: bytes sent, or -1 on error
 */
int net_send_tcp(const char *ip, int port, const void *data, int len);

#endif /* __NET_H__ */