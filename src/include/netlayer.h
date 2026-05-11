/*
 * =====================================================================================
 *
 *       Filename:  netlayer.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2014年08月26日 15时55分53秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  jianxi sun (jianxi), ycsunjane@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef __NETLAYER_H__
#define __NETLAYER_H__
#include <netinet/in.h>
#include "capwap/capwap.h"

#define NET_PKT_DATALEN 	(2048)

struct nettcp_t {
	int sock;
	struct sockaddr_in addr;
};

/* TCP network layer */
int tcp_connect(struct nettcp_t *tcp);
int tcp_rcv(struct nettcp_t *tcp, char *data, int size);
int tcp_rcv_msg(struct nettcp_t *tcp, char *data, int bufsize);
int tcp_sendpkt(struct nettcp_t *tcp, char *data, int size);
void tcp_close(struct nettcp_t *tcp);
#ifdef SERVER
int tcp_listen(struct nettcp_t *tcp);
int tcp_accept(struct nettcp_t *tcp, void *func(void *));
#endif

/* CAPWAP network layer */
int capwap_net_init(int mode, const char *ac_ip, int ac_port);
void capwap_net_set_msg_handler(int (*handler)(char *data, int len, void *arg), void *arg);
int capwap_net_send(int msg_type, char *data, int len);
int capwap_net_start(void);
int capwap_net_stop(void);
enum capwap_state capwap_net_get_state(void);
void capwap_net_set_state(enum capwap_state state);
struct sockaddr_in *capwap_net_get_ac_addr(void);
struct sockaddr_in *capwap_net_get_wtp_addr(void);
#endif /* __NETLAYER_H__ */
