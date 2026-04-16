/*
 * =====================================================================================
 *       Filename:  apstatus.h
 *       Description:  AP status structures for multi-SSID and dual-band support
 * =====================================================================================
 */
#ifndef __APSTATUS_H__
#define __APSTATUS_H__

#define SSID_MAXLEN  256
#define MAX_RADIOS   4
#define MAX_SSIDS_PER_RADIO  8
#define MAX_TOTAL_SSIDS  (MAX_RADIOS * MAX_SSIDS_PER_RADIO)

#define FREQ_BAND_2G   "2.4GHz"
#define FREQ_BAND_5G   "5GHz"
#define FREQ_BAND_6G   "6GHz"
#define FREQ_BAND_UNKNOWN "Unknown"

enum wifi_protocol {
	WIFI_80211_A   = 1,
	WIFI_80211_B   = 2,
	WIFI_80211_G   = 3,
	WIFI_80211_N   = 4,
	WIFI_80211_AC  = 5,
	WIFI_80211_AX  = 6,
};

struct ssid_info {
	char ssid[SSID_MAXLEN];
	int  power;
	int  channel;
	int  clients;
	char band[16];
	enum wifi_protocol protocol;
	int  enabled;
};

struct apstatus_t {
	int  ssidnum;
	struct ssid_info ssids[MAX_TOTAL_SSIDS];
};

struct apstatus_t *get_apstatus(void);
unsigned long get_uptime(void);
unsigned long get_memfree(void);
unsigned int  get_cpu_usage(void);

#endif /* __APSTATUS_H__ */