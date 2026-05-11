/*
 * ============================================================================
 *
 *       Filename:  apstatus.c
 *
 *    Description:  AP system status collection.
 *                  Supports multi-SSID and dual-band (2.4GHz/5GHz/6GHz).
 *                  Also supports single-band (2.4GHz-only or 5GHz-only) APs.
 *                  Gathers real system information from /proc and UCI.
 *
 *        Version:  2.1
 *        Created:  2026-04-12
 *       Revision:  dual-band and multi-SSID support, single-band compatibility
 *       Compiler:  gcc
 *
 * ============================================================================
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/statvfs.h>
#include <ctype.h>
#include <time.h>

#include "apstatus.h"
#include "log.h"
#include <sys/wait.h>
#include <fcntl.h>

static struct apstatus_t cached_status;
static time_t cache_time = 0;
#define CACHE_TTL  5

static int read_proc(const char *path, char *buf, size_t buflen)
{
	FILE *fp = fopen(path, "r");
	if (!fp)
		return -1;

	size_t r = fread(buf, 1, buflen - 1, fp);
	buf[r] = '\0';
	fclose(fp);

	while (r > 0 && (buf[r-1] == '\n' || buf[r-1] == '\r'))
		buf[--r] = '\0';

	return (int)r;
}

static unsigned long get_uptime_sec(void)
{
	char buf[64];
	if (read_proc("/proc/uptime", buf, sizeof(buf)) > 0) {
		return (unsigned long)atof(buf);
	}
	return 0;
}

static unsigned long get_memfree_kb(void)
{
	char buf[256];
	if (read_proc("/proc/meminfo", buf, sizeof(buf)) > 0) {
		char *p = strstr(buf, "MemFree:");
		if (p) {
			unsigned long kb;
			if (sscanf(p, "MemFree: %lu", &kb) == 1)
				return kb;
		}
	}
	return 0;
}

static unsigned int get_cpu_percent(void)
{
	static unsigned long prev_total = 0;
	static unsigned long prev_idle = 0;
	char buf[256];
	unsigned long user, nice, system, idle, iowait, irq, softirq;
	unsigned long total, total_d, idle_d;
	unsigned int pct;

	if (read_proc("/proc/stat", buf, sizeof(buf)) <= 0)
		return 0;

	if (sscanf(buf, "cpu  %lu %lu %lu %lu %lu %lu %lu",
		&user, &nice, &system, &idle, &iowait, &irq, &softirq) != 7)
		return 0;

	total = user + nice + system + idle + iowait + irq + softirq;
	idle = idle + iowait;

	total_d = total - prev_total;
	idle_d = idle - prev_idle;

	prev_total = total;
	prev_idle = idle;

	if (total_d == 0)
		return 0;

	pct = (unsigned int)((total_d - idle_d) * 100 / total_d);
	if (pct > 100) pct = 100;

	return pct;
}

static void get_radio_frequency_band(const char *iface, char *band, size_t band_len)
{
	char cmd[128];
	char freq_buf[32];

	snprintf(cmd, sizeof(cmd),
		"iw dev %s info 2>/dev/null | grep -E 'channel|freq' | head -1 | awk '{print $2}'",
		iface);

	FILE *fp = popen(cmd, "r");
	int freq = 0;
	if (fp && fgets(freq_buf, sizeof(freq_buf), fp)) {
		freq = atoi(freq_buf);
	}
	if (fp) pclose(fp);

	if (freq >= 5150 && freq < 5900)
		strncpy(band, FREQ_BAND_5G, band_len - 1);
	else if (freq >= 5900)
		strncpy(band, FREQ_BAND_6G, band_len - 1);
	else if (freq >= 2400 && freq < 2500)
		strncpy(band, FREQ_BAND_2G, band_len - 1);
	else
		strncpy(band, FREQ_BAND_UNKNOWN, band_len - 1);
	band[band_len - 1] = '\0';
}

static int get_channel(const char *iface)
{
	char cmd[128];
	char chan_buf[16];

	snprintf(cmd, sizeof(cmd),
		"iw dev %s info 2>/dev/null | grep 'channel' | head -1 | awk '{print $2}'",
		iface);

	FILE *fp = popen(cmd, "r");
	int channel = 0;
	if (fp && fgets(chan_buf, sizeof(chan_buf), fp)) {
		channel = atoi(chan_buf);
	}
	if (fp) pclose(fp);
	return channel;
}

static int get_signal_strength(const char *iface)
{
	char cmd[128];
	char sig_buf[32];

	snprintf(cmd, sizeof(cmd),
		"iw dev %s link 2>/dev/null | grep 'signal:' | awk '{print $2}'",
		iface);

	FILE *fp = popen(cmd, "r");
	int signal = -100;
	if (fp && fgets(sig_buf, sizeof(sig_buf), fp)) {
		signal = atoi(sig_buf);
	}
	if (fp) pclose(fp);
	return signal;
}

static int get_connected_clients(const char *iface)
{
	char cmd[128];
	char sta_buf[16];

	snprintf(cmd, sizeof(cmd),
		"iw dev %s station dump 2>/dev/null | grep -c 'Station' || echo 0",
		iface);

	FILE *fp = popen(cmd, "r");
	int clients = 0;
	if (fp && fgets(sta_buf, sizeof(sta_buf), fp)) {
		clients = atoi(sta_buf);
	}
	if (fp) pclose(fp);
	return clients;
}

static enum wifi_protocol get_protocol(const char *iface)
{
	char cmd[256];
	char buf[64];
	enum wifi_protocol proto = WIFI_80211_B;

	snprintf(cmd, sizeof(cmd),
		"iw dev %s info 2>/dev/null", iface);

	FILE *fp = popen(cmd, "r");
	if (!fp) return proto;

	while (fgets(buf, sizeof(buf), fp)) {
		if (strstr(buf, "VHT")) {
			proto = WIFI_80211_AC;
			break;
		} else if (strstr(buf, "HE")) {
			proto = WIFI_80211_AX;
			break;
		} else if (strstr(buf, "HT")) {
			proto = WIFI_80211_N;
			break;
		} else if (strstr(buf, "5 GHz")) {
			proto = WIFI_80211_A;
			break;
		}
	}
	pclose(fp);
	return proto;
}

static int __attribute__((unused)) get_wifi_interface_count(void)
{
	FILE *fp = popen(
		"ls /sys/class/net/ 2>/dev/null | grep -cE '^wl|^wifi|^ra|^ath' || "
		"iw dev 2>/dev/null | grep -c 'Interface' || echo 0", "r");
	int count = 0;
	char buf[16];
	if (fp && fgets(buf, sizeof(buf), fp)) {
		count = atoi(buf);
	}
	if (fp) pclose(fp);
	return count;
}

static int __attribute__((unused)) get_all_wifi_interfaces(char interfaces[][32], int max_interfaces)
{
	int count = 0;

	FILE *fp = popen("ls /sys/class/net/ 2>/dev/null", "r");
	if (!fp) {
		fp = popen("iw dev 2>/dev/null | grep 'Interface' | awk '{print $2}'", "r");
		if (!fp) return 0;
		while (fgets(interfaces[count], 32, fp) && count < max_interfaces) {
			size_t len = strlen(interfaces[count]);
			while (len > 0 && isspace(interfaces[count][len-1]))
				interfaces[count][--len] = '\0';
			if (interfaces[count][0] != '\0')
				count++;
		}
		pclose(fp);
		return count;
	}

	char buf[64];
	while (fgets(buf, sizeof(buf), fp) && count < max_interfaces) {
		size_t len = strlen(buf);
		while (len > 0 && isspace(buf[len-1])) buf[--len] = '\0';

		if ((strncmp(buf, "wl", 2) == 0 || strncmp(buf, "wifi", 4) == 0 ||
		     strncmp(buf, "ra", 2) == 0 || strncmp(buf, "ath", 3) == 0 ||
		     strncmp(buf, "wlan", 4) == 0) &&
		    (isdigit(buf[len-1]) || buf[len-1] == '_')) {
			strncpy(interfaces[count], buf, 31);
			interfaces[count][31] = '\0';
			count++;
		}
	}
	pclose(fp);
	return count;
}

static int get_wireless_iface_count_uci(void)
{
	FILE *fp = popen("uci show wireless 2>/dev/null | grep -c 'wifi-iface' || echo 0", "r");
	int count = 0;
	char buf[16];
	if (fp && fgets(buf, sizeof(buf), fp)) {
		count = atoi(buf);
	}
	if (fp) pclose(fp);
	return count;
}

static int get_ssid_for_interface_by_idx(int idx, char *ssid, size_t ssid_len)
{
	char cmd[128];
	char buf[256];
	char *start, *end;

	snprintf(cmd, sizeof(cmd),
		"uci get wireless.@wifi-iface[%d].ssid 2>/dev/null", idx);

	FILE *fp = popen(cmd, "r");
	if (!fp) return -1;

	if (fgets(buf, sizeof(buf), fp)) {
		size_t len = strlen(buf);
		while (len > 0 && isspace(buf[len-1])) buf[--len] = '\0';

		if (buf[0] == '\'' || buf[0] == '"') {
			start = buf + 1;
			end = strchr(start, buf[0]);
			if (end) *end = '\0';
		} else {
			start = buf;
		}

		if (start[0] != '\0') {
			strncpy(ssid, start, ssid_len - 1);
			ssid[ssid_len - 1] = '\0';
			pclose(fp);
			return 0;
		}
	}
	pclose(fp);
	return -1;
}

static int is_interface_enabled_by_idx(int idx)
{
	char cmd[128];
	char buf[16];

	snprintf(cmd, sizeof(cmd),
		"uci get wireless.@wifi-iface[%d].disabled 2>/dev/null || echo 0", idx);

	FILE *fp = popen(cmd, "r");
	int disabled = 1;
	if (fp && fgets(buf, sizeof(buf), fp)) {
		disabled = atoi(buf);
	}
	if (fp) pclose(fp);
	return (disabled == 0) ? 1 : 0;
}

static int get_device_for_iface_idx(int idx, char *device, size_t dev_len)
{
	char cmd[128];
	char buf[64];

	snprintf(cmd, sizeof(cmd),
		"uci get wireless.@wifi-iface[%d].device 2>/dev/null || echo ''", idx);

	FILE *fp = popen(cmd, "r");
	if (fp && fgets(buf, sizeof(buf), fp)) {
		size_t len = strlen(buf);
		while (len > 0 && isspace(buf[len-1])) buf[--len] = '\0';
		if (buf[0] != '\0') {
			strncpy(device, buf, dev_len - 1);
			device[dev_len - 1] = '\0';
			pclose(fp);
			return 0;
		}
	}
	if (fp) pclose(fp);
	return -1;
}

struct apstatus_t *get_apstatus(void)
{
	time_t now = time(NULL);

	if (now - cache_time < CACHE_TTL)
		return &cached_status;

	memset(&cached_status, 0, sizeof(cached_status));

	int wifi_count = get_wireless_iface_count_uci();

	if (wifi_count == 0) {
		strncpy(cached_status.ssids[0].ssid, "No-WiFi",
			sizeof(cached_status.ssids[0].ssid) - 1);
		strncpy(cached_status.ssids[0].band, FREQ_BAND_UNKNOWN,
			sizeof(cached_status.ssids[0].band) - 1);
		cached_status.ssidnum = 1;
		cached_status.ssids[0].enabled = 0;
		cache_time = now;
		return &cached_status;
	}

	cached_status.ssidnum = 0;

	for (int i = 0; i < wifi_count && cached_status.ssidnum < MAX_TOTAL_SSIDS; i++) {
		struct ssid_info *si = &cached_status.ssids[cached_status.ssidnum];

		char device[32] = {0};
		get_device_for_iface_idx(i, device, sizeof(device));

		if (get_ssid_for_interface_by_idx(i, si->ssid, sizeof(si->ssid)) != 0) {
			snprintf(si->ssid, sizeof(si->ssid), "SSID-%d", i + 1);
		}

		si->channel = 0;
		si->power = -100;
		si->clients = 0;
		si->enabled = is_interface_enabled_by_idx(i);
		strncpy(si->band, FREQ_BAND_UNKNOWN, sizeof(si->band) - 1);
		si->protocol = WIFI_80211_B;

		char cmd[512];
		snprintf(cmd, sizeof(cmd),
			"iw dev 2>/dev/null | grep -A2 'Interface %s' | grep 'ssid' | head -1 | awk '{print $2}'",
			device);

		FILE *fp = popen(cmd, "r");
		if (fp) {
			char ibuf[64];
			if (fgets(ibuf, sizeof(ibuf), fp)) {
				size_t len = strlen(ibuf);
				while (len > 0 && isspace(ibuf[len-1])) ibuf[--len] = '\0';
				if (ibuf[0] != '\0') {
					strncpy(si->ssid, ibuf, sizeof(si->ssid) - 1);
					si->ssid[sizeof(si->ssid) - 1] = '\0';
				}
			}
			pclose(fp);
		}

		char iface_name[32] = {0};
		char cmd2[512];
		snprintf(cmd2, sizeof(cmd2),
			"iw dev 2>/dev/null | grep -B3 'ssid %s' | grep 'Interface' | awk '{print $2}'",
			si->ssid);
		fp = popen(cmd2, "r");
		if (fp && fgets(iface_name, sizeof(iface_name), fp)) {
			size_t len = strlen(iface_name);
			while (len > 0 && isspace(iface_name[len-1]))
				iface_name[--len] = '\0';

			if (iface_name[0] != '\0') {
				si->channel = get_channel(iface_name);
				si->power = get_signal_strength(iface_name);
				si->clients = get_connected_clients(iface_name);
				get_radio_frequency_band(iface_name, si->band, sizeof(si->band));
				si->protocol = get_protocol(iface_name);
			}
		}
		if (fp) pclose(fp);

		cached_status.ssidnum++;
	}

	if (cached_status.ssidnum == 0) {
		strncpy(cached_status.ssids[0].ssid, "OpenWrt-AP",
			sizeof(cached_status.ssids[0].ssid) - 1);
		strncpy(cached_status.ssids[0].band, FREQ_BAND_2G,
			sizeof(cached_status.ssids[0].band) - 1);
		cached_status.ssidnum = 1;
		cached_status.ssids[0].enabled = 1;
	}

	cache_time = now;
	return &cached_status;
}

unsigned long get_uptime(void)
{
	return get_uptime_sec();
}

unsigned long get_memfree(void)
{
	return get_memfree_kb();
}

unsigned int get_cpu_usage(void)
{
	return get_cpu_percent();
}