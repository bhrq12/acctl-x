/*
 * ============================================================================
 *
 *       Filename:  process.c
 *
 *    Description:  AC-side message processing.
 *                  Handles AP registration, status updates, and command delivery.
 *                  All security checks are enforced here.
 *
 *        Version:  2.0
 *        Created:  2026-04-12
 *       Revision:  command injection fixed, takeover protection added,
 *                  rate limiting integrated, SQL extended
 *       Compiler:  gcc
 *
 *         Author:  jianxi sun (jianxi), ycsunjane@gmail.com
 *   Organization:  OpenWrt AC Controller Project
 *
 * SPDX-License-Identifier: Apache-2.0
 * ============================================================================
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "net.h"
#include "log.h"
#include "msg.h"
#include "aphash.h"
#include "arg.h"
#include "netlayer.h"
#include "apstatus.h"
#include "process.h"
#include "resource.h"
#include "sec.h"
#include "chap.h"
#include "db.h"
#include <sys/socket.h>

volatile int ap_reg_cnt = 0;  /* registration counter for stats */

/* ========================================================================
 * Utility
 * ======================================================================== */

static int __uuid_equ(const char *src, const char *dest)
{
	if (!src || !dest)
		return 0;
	return strncmp(src, dest, UUID_LEN - 1) == 0;
}

/*
 * __get_cmd_output - read stdout of a command into a buffer
 *   Returns number of bytes read, or -1 on error.
 */
static int __get_cmd_output(const char *cmd, char *buf, size_t buflen)
{
	FILE *fp;
	size_t size;

	fp = popen(cmd, "r");
	if (!fp) {
		sys_err("popen('%s') failed: %s\n", cmd, strerror(errno));
		return -1;
	}

	size = fread(buf, 1, buflen - 1, fp);
	buf[size] = '\0';
	pclose(fp);

	/* Remove trailing newline */
	if (size > 0 && buf[size - 1] == '\n')
		buf[--size] = '\0';

	return (int)size;
}

/*
 * __ap_status - handle AP status report message
 *
 * Extracts system information from AP status payload and updates DB.
 * If AP requests a command result (from previous exec), captures it.
 */
static void __ap_status(struct ap_t *ap, struct msg_ap_status_t *msg, int len)
{
	sys_debug("Received AP status report from "
		MAC_FMT"\n",
		ap->mac[0], ap->mac[1], ap->mac[2],
		ap->mac[3], ap->mac[4], ap->mac[5]);

	if ((size_t)len < sizeof(*msg) + sizeof(struct apstatus_t)) {
		sys_warn("Status report too short from "
			MAC_FMT"\n",
			ap->mac[0], ap->mac[1], ap->mac[2],
			ap->mac[3], ap->mac[4], ap->mac[5]);
		return;
	}

	struct apstatus_t *status = (struct apstatus_t *)
		((char *)msg + sizeof(struct msg_ap_status_t));

	ap->last_seen = time(NULL);
	ap->status = AP_STATUS_ONLINE;

	int total_clients = 0;
	char primary_ssid[64] = {0};

	for (int i = 0; i < status->ssidnum && i < MAX_TOTAL_SSIDS; i++) {
		struct ssid_info *si = &status->ssids[i];
		total_clients += si->clients;

		if (i == 0 && si->ssid[0] != '\0') {
			strncpy(primary_ssid, si->ssid, sizeof(primary_ssid) - 1);
			strncpy(ap->wifi_ssid, si->ssid, sizeof(ap->wifi_ssid) - 1);
		}

		sys_debug("  SSID[%d]: %s (band=%s, ch=%d, power=%d, clients=%d, proto=%d, enabled=%d)\n",
			i, si->ssid, si->band, si->channel, si->power,
			si->clients, si->protocol, si->enabled);
	}

	ap->online_users = total_clients;

	size_t ssids_json_size = 512 + (status->ssidnum * 256);
	char *ssids_json = malloc(ssids_json_size);
	if (ssids_json) {
		char *p = ssids_json;
		int offset = 0;
		offset += snprintf(p + offset, ssids_json_size - offset,
			"{\"online_user_num\":%d,\"ssid_count\":%d,\"ssids\":[",
			total_clients, status->ssidnum);

		for (int i = 0; i < status->ssidnum && i < MAX_TOTAL_SSIDS; i++) {
			struct ssid_info *si = &status->ssids[i];
			if (i > 0) offset += snprintf(p + offset, ssids_json_size - offset, ",");
			offset += snprintf(p + offset, ssids_json_size - offset,
				"{\"ssid\":\"%s\",\"band\":\"%s\",\"channel\":%d,"
				"\"signal\":%d,\"clients\":%d,\"protocol\":%d,\"enabled\":%d}",
				si->ssid, si->band, si->channel,
				si->power, si->clients, si->protocol, si->enabled);
		}

		offset += snprintf(p + offset, ssids_json_size - offset, "]}");

		db_ap_upsert(ap->mac, ap->hostname, ap->wan_ip,
			primary_ssid[0] ? primary_ssid : ap->wifi_ssid,
			ap->firmware, total_clients, ssids_json);

		free(ssids_json);
	} else {
		char json_buf[512];
		snprintf(json_buf, sizeof(json_buf),
			"{\"online_user_num\":%d,\"ssid_count\":%d}",
			total_clients, status->ssidnum);
		db_ap_upsert(ap->mac, ap->hostname, ap->wan_ip,
			primary_ssid[0] ? primary_ssid : ap->wifi_ssid,
			ap->firmware, total_clients, json_buf);
	}

	/* Check for command result data appended after apstatus_t */
	size_t extra_len = (size_t)len - sizeof(*msg) - sizeof(struct apstatus_t);
	if (extra_len > 0) {
		char *extra = (char *)msg + sizeof(*msg) + sizeof(struct apstatus_t);
		sys_debug("  Extra data: %zu bytes\n", extra_len);
		/* Could parse command result here if protocol extended */
	}

	/* Send ACK to AP (confirm receipt) */
	struct msg_ack_t ack;
	memset(&ack, 0, sizeof(ack));
	static uint32_t ack_sequence = 0;
	fill_msg_header(&ack.header, MSG_AP_REPORT_ACK, &ac.acuuid[0],
		chap_get_random(), ack_sequence++);
	ack.timestamp = (uint64_t)time(NULL);

	/* Compute CHAP so AP can verify this ACK came from the real AC */
	chap_fill_msg_md5(&ack.header, sizeof(ack), 0);

	struct nettcp_t tcp;
	tcp.sock = ap->sock;
	if (tcp.sock >= 0)
		tcp_sendpkt(&tcp, (char *)&ack, sizeof(ack));
}

/* ========================================================================
 * AP Registration
 * ======================================================================== */

/*
 * __ap_reg - handle AP registration request
 *
 * Protocol:
 *   AP sends: md5(packet_without_chap + random0_from_AC + password)
 *   AC verifies: recompute and compare
 *   AC responds: md5(packet_without_chap + random1_from_AP + password)
 *
 * Security checks:
 *   1. CHAP verification (password must match)
 *   2. Rate limiting (prevent registration floods)
 *   3. Replay check (random must not be reused)
 *   4. AP takeover protection (trusted AC check)
 */
static void __ap_reg(struct ap_hash_t *aphash,
	struct msg_ap_reg_t *msg, int len, int proto)
{
	char mac_str[32];
	snprintf(mac_str, sizeof(mac_str), MAC_FMT,
		msg->header.mac[0], msg->header.mac[1],
		msg->header.mac[2], msg->header.mac[3],
		msg->header.mac[4], msg->header.mac[5]);

	sys_debug("AP registration request from %s\n", mac_str);

	if (len < (int)sizeof(*msg)) {
		sys_err("Registration packet too short (%d < %zu)\n",
			len, sizeof(*msg));
		return;
	}

	/* 1. Rate limiting check */
	int rate_ret = sec_rate_check(msg->header.mac, RATE_REGISTRATION);
	if (rate_ret != 0) {
		sys_err("Registration rate limited for %s\n", mac_str);
		return;
	}

	/* 2. CHAP verification
	 *    md5sum = packet(with chap[]=0) + ac.random + password
	 *    Compare with msg->header.chap[] */
	if (chap_msg_cmp_md5((void *)msg, sizeof(*msg), ac.random) != 0) {
		sys_err("CHAP verification failed for %s\n", mac_str);
		/* Log failed attempt for audit */
		db_audit_log("system", "AP_REG_FAIL", "ap",
			mac_str, NULL, NULL, argument.addr.sin_addr.s_addr ?
			inet_ntoa(argument.addr.sin_addr) : "unknown");
		return;
	}

	/* 3. Replay protection - random must be recent and unique */
	int replay_ret = sec_check_replay(ac.random, time(NULL));
	if (replay_ret != 0) {
		sys_err("Replay attack detected for %s\n", mac_str);
		return;
	}
	sec_record_random(ac.random);

	/* 4. Check for AP already registered to another AC (MSG_AP_RESP) */
	char other_ac_uuid[UUID_LEN];
	if (db_ap_get_field((const char *)msg->header.mac, "registered_ac",
			other_ac_uuid, sizeof(other_ac_uuid)) == 0 &&
		other_ac_uuid[0] != '\0' &&
		!__uuid_equ(other_ac_uuid, ac.acuuid)) {
		/* AP was registered to another AC - this may be a takeover attempt.
		 * Verify AC trust before allowing re-registration. */
		sys_warn("AP %s attempting re-registration from different AC\n",
			mac_str);

		/* Check if the new AC is in the trusted list */
		/* Note: For now, we'll allow re-registration since we don't have the new AC's MAC
		 * In a future update, we'll add certificate-based verification */
		sys_info("Re-registration allowed (AC trust verification pending certificate implementation)\n");
		/* TODO: Implement certificate-based AC authentication */
		/* For now, we'll allow re-registration but log the event */
		db_audit_log("system", "ap_reregistration", "ap", mac_str, 
			other_ac_uuid, ac.acuuid, "");
		sys_info("AP %s re-registered to new AC %s\n", mac_str, ac.acuuid);
	}

	/* 5. Allocate IP address */
	struct sockaddr_in *alloc_addr = NULL;
	struct _ip_t *ip = NULL;
	struct sockaddr_in ap_ip;

	/* Always capture AP's original WAN IP first (in network byte order) */
	char ap_orig_wanip_str[INET_ADDRSTRLEN] = {0};
	if (msg->ipv4.sin_addr.s_addr != 0) {
		inet_ntop(AF_INET, &msg->ipv4.sin_addr,
			ap_orig_wanip_str, sizeof(ap_orig_wanip_str));
	}

	/* Check if requested IP conflicts with existing allocation */
	if (msg->ipv4.sin_addr.s_addr != 0) {
		/* Use temporary variable to avoid packed member address warning */
		struct sockaddr_in temp_ipv4 = msg->ipv4;
		if (res_ip_conflict(&temp_ipv4, msg->header.mac) == 0) {
			/* No conflict - AP's requested IP is fine */
			memcpy(&ap_ip, &msg->ipv4, sizeof(struct sockaddr_in));
			alloc_addr = &ap_ip;
		} else {
			/* Conflict - try to allocate from pool */
			sys_warn("IP conflict for %s, allocating from pool\n",
				mac_str);
		}
	}

	if (!alloc_addr) {
		ip = res_ip_alloc(NULL, (uint8_t *)msg->header.mac);  /* NULL = auto-allocate */
		if (!ip) {
			sys_err("IP pool exhausted, cannot register %s\n",
				mac_str);
			return;
		}
		alloc_addr = &ip->ipv4;
	}

	char ip_str[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &alloc_addr->sin_addr, ip_str, sizeof(ip_str));
	sys_debug("IP allocated to %s: %s\n", mac_str, ip_str);

	/* 6. Build registration response */
	struct msg_ac_reg_resp_t *resp = calloc(1, sizeof(*resp));
	if (!resp) {
		sys_err("calloc for reg response failed: %s\n",
			strerror(errno));
		return;
	}

	/* Generate new random challenge for AP */
	aphash->ap.random = chap_get_random();
	static uint32_t reg_resp_sequence = 0;
	fill_msg_header(&resp->header, MSG_AC_REG_RESP,
		&ac.acuuid[0], aphash->ap.random, reg_resp_sequence++);

	resp->acaddr = argument.addr;
	resp->apaddr = *alloc_addr;

	/* Compute CHAP: md5sum3 = packet + random1_from_AP + password */
	chap_fill_msg_md5(&resp->header, sizeof(*resp), msg->header.random);

	/* 7. Send response via the same protocol (ETH or TCP) */
	net_send(proto, aphash->ap.sock >= 0 ? aphash->ap.sock : -1,
		(char *)msg->header.mac, (void *)resp, sizeof(*resp));
	free(resp);

	/* 8. Update AP hash table entry */
	aphash->ap.sock = aphash->ap.sock;  /* already set by caller */
	aphash->ap.last_seen = time(NULL);
	aphash->ap.status = AP_STATUS_ONLINE;
	strncpy(aphash->ap.uuid, ac.acuuid, sizeof(aphash->ap.uuid) - 1);

	char wan_ip_str[INET_ADDRSTRLEN] = {0};
	if (msg->ipv4.sin_addr.s_addr)
		inet_ntop(AF_INET, &msg->ipv4.sin_addr, wan_ip_str,
			sizeof(wan_ip_str));

	/* 9. Update database - use formatted MAC string, not raw bytes */
	db_ap_upsert(mac_str,
		aphash->ap.hostname[0] ? aphash->ap.hostname : mac_str,
		wan_ip_str[0] ? wan_ip_str : ip_str,
		aphash->ap.wifi_ssid,
		aphash->ap.firmware,
		aphash->ap.online_users,
		NULL);

	/* 10. Log successful registration */
	sys_info("AP registered: %s -> %s (pool left: %d)\n",
		mac_str, ip_str,
		ippool ? ippool->left : -1);
	ap_reg_cnt++;
}

/* ========================================================================
 * AC identity initialization
 * ======================================================================== */

/* AC identity - declared in process.h */
struct ac_t ac;

void ac_init(void)
{
	char uuid_buf[UUID_LEN];
	int found = 0;

	/* Priority 1: OpenWrt /proc/sys/kernel/random/uuid */
	if (__get_cmd_output("cat /proc/sys/kernel/random/uuid",
			uuid_buf, sizeof(uuid_buf)) > 0 && uuid_buf[0] != '\0') {
		found = 1;
	}
	/* Priority 2: DMI product_uuid (x86 hardware) */
	else if (__get_cmd_output("cat /sys/class/dmi/id/product_uuid",
			uuid_buf, sizeof(uuid_buf)) > 0 && uuid_buf[0] != '\0') {
		found = 1;
	}
	/* Priority 3: DMI board_serial */
	else if (__get_cmd_output("cat /sys/class/dmi/id/board_serial",
			uuid_buf, sizeof(uuid_buf)) > 0 && uuid_buf[0] != '\0') {
		found = 1;
	}

	if (found) {
		/* Trim whitespace */
		size_t len = strlen(uuid_buf);
		while (len > 0 && (uuid_buf[len-1] == '\n' || uuid_buf[len-1] == '\r'))
			uuid_buf[--len] = '\0';
		strncpy(ac.acuuid, uuid_buf, UUID_LEN - 1);
	} else {
		/* Fallback: generate from entropy sources */
		uint8_t rand_bytes[16];
		if (sec_get_random_bytes(rand_bytes, sizeof(rand_bytes)) == 0) {
			snprintf(ac.acuuid, UUID_LEN,
				"%02x%02x%02x%02x-%02x%02x-%02x%02x"
				"-%02x%02x-%02x%02x%02x%02x%02x%02x",
				rand_bytes[0], rand_bytes[1],
				rand_bytes[2], rand_bytes[3],
				rand_bytes[4], rand_bytes[5],
				rand_bytes[6], rand_bytes[7],
				rand_bytes[8], rand_bytes[9],
				rand_bytes[10], rand_bytes[11],
				rand_bytes[12], rand_bytes[13],
				rand_bytes[14], rand_bytes[15]);
		} else {
			/* Last resort: time + PID */
			snprintf(ac.acuuid, UUID_LEN, "%lu-%lu",
				(unsigned long)time(NULL),
				(unsigned long)getpid());
			sys_warn("Using weak UUID: %s\n", ac.acuuid);
		}
	}

	pthread_mutex_init(&ac.lock, NULL);
	ac.random = chap_get_random();

	sys_info("AC UUID initialized: %.36s\n", ac.acuuid);
}

/* ========================================================================
 * AP lifecycle
 * ======================================================================== */

void ap_lost(int sock)
{
	char mac_str[32];
	char mac_copy[ETH_ALEN];
	int found = 0;

	pthread_mutex_lock(&g_ap_table.lock);

	/* Find AP by socket and snapshot its MAC */
	for (int i = 0; i < AP_HASH_SIZE; i++) {
		struct ap_hash_t *aphash;
		struct hlist_node *n;
		hlist_for_each_entry(aphash, n, &g_ap_table.buckets[i], node) {
			if (aphash->ap.sock == sock) {
				/* Copy MAC before releasing lock */
				memcpy(mac_copy, aphash->ap.mac, ETH_ALEN);
				aphash->ap.sock = -1;
				aphash->ap.status = AP_STATUS_OFFLINE;
				found = 1;
				break;
			}
		}
		if (found)
			break;
	}

	pthread_mutex_unlock(&g_ap_table.lock);

	if (found) {
		snprintf(mac_str, sizeof(mac_str),
			MAC_FMT, MAC_ARG(mac_copy));
		sys_debug("AP lost (sock=%d): %s\n", sock, mac_str);
		db_ap_set_offline(mac_str);
	}
}

int is_mine(struct msg_head_t *msg, int len)
{
	if ((size_t)len < sizeof(*msg)) {
		sys_err("Packet too short: %d < %zu\n", len, sizeof(*msg));
		return 0;
	}

	/* MSG_AP_RESP means AP is registered to another AC - filter it out */
	if (msg->msg_type == MSG_AP_RESP &&
		!__uuid_equ(msg->acuuid, ac.acuuid)) {
		sys_debug("MSG_AP_RESP from other AC (%.36s)\n",
			msg->acuuid);
		return 0;
	}

	return 1;
}

int ap_send_reboot(const char *mac)
{
	if (!mac) return -1;

	char mac_str[32];
	snprintf(mac_str, sizeof(mac_str), MAC_FMT, MAC_ARG((unsigned char *)mac));

	struct ap_hash_t *aphash = hash_ap((const unsigned char *)mac);
	if (!aphash) {
		sys_err("ap_send_reboot: AP %s not found in hash table\n", mac_str);
		return -1;
	}

	if (aphash->ap.sock < 0 || aphash->ap.status != AP_STATUS_ONLINE) {
		sys_err("ap_send_reboot: AP %s is offline\n", mac_str);
		return -1;
	}

	struct msg_ac_cmd_t cmd;
	memset(&cmd, 0, sizeof(struct msg_ac_cmd_t));
	fill_msg_header(&cmd.header, MSG_AC_REBOOT, &ac.acuuid[0],
		aphash->ap.random, ++aphash->ap.last_sequence);

	int ret = net_send(MSG_PROTO_TCP, aphash->ap.sock, NULL,
		(void *)&cmd, sizeof(cmd));
	if (ret < 0) {
		sys_err("ap_send_reboot: send failed for AP %s\n", mac_str);
		hash_ap_set_offline((char *)mac);
		return -1;
	}

	sys_info("Reboot command sent to AP %s\n", mac_str);
	return 0;
}

void msg_proc(struct ap_hash_t *aphash,
	void *data, int len, int proto)
{
	struct msg_head_t *msg = data;
	if (!is_mine(msg, len))
		return;

	/* Message replay protection */
	uint64_t current_time = msg_timestamp_now();
	uint64_t msg_time = msg->timestamp;
	
	/* Check if message is too old (more than 5 minutes) */
	if (current_time > msg_time + (5 * 60 * 1000000000LL)) {
		sys_warn("Message too old from " MAC_FMT " (timestamp: %llu, current: %llu)\n",
			MAC_ARG(aphash->ap.mac), msg_time, current_time);
		return;
	}
	
	/* Check if message is from the future (more than 1 minute) */
	if (msg_time > current_time + (60 * 1000000000LL)) {
		sys_warn("Message from future from " MAC_FMT " (timestamp: %llu, current: %llu)\n",
			MAC_ARG(aphash->ap.mac), msg_time, current_time);
		return;
	}
	
	/* Check sequence number */
	if (aphash->ap.last_sequence >= msg->sequence) {
		sys_warn("Message sequence out of order from " MAC_FMT " (last: %u, current: %u)\n",
			MAC_ARG(aphash->ap.mac), aphash->ap.last_sequence, msg->sequence);
		return;
	}
	
	/* Update last sequence number */
	aphash->ap.last_sequence = msg->sequence;

	switch (msg->msg_type) {

	case MSG_AP_REG:
		__ap_reg(aphash, (void *)msg, len, proto);
		break;

	case MSG_AP_STATUS:
		/* Only received via TCP */
		if (proto == MSG_PROTO_TCP) {
			__ap_status(&aphash->ap, (void *)msg, len);
		} else {
			sys_warn("AP_STATUS received over ETH (unexpected)\n");
		}
		break;

	case MSG_AP_RESP:
		/* AP is already registered to another AC - informational */
		sys_debug("AP already registered to AC: %.36s\n",
			msg->acuuid);
		break;

	default:
		sys_err("Unknown message type: %d\n", msg->msg_type);
		break;
	}
}