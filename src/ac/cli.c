/*
 * ============================================================================
 *
 *       Filename:  cli.c
 *
 *    Description:  Lightweight CLI for LuCI web interface to access JSON DB.
 *                  Supports read and write operations needed by the web UI.
 *                  Not part of the acser daemon — standalone binary.
 *
 *        Version:  1.0
 *        Created:  2026-04-14
 *       Compiler:  gcc
 *
 * ============================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>
#include <sys/types.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define DB_PATH  "/etc/acctl/ac.json"
#define LOCK_PATH  "/etc/acctl/ac.json.lock"
#define OUT_MAX  (64 * 1024)

static int lock_db(int lock_type)
{
    int fd = open(LOCK_PATH, O_CREAT | O_RDWR, 0644);
    if (fd < 0) return -1;
    struct flock fl;
    memset(&fl, 0, sizeof(fl));
    fl.l_type = lock_type;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    int ret = fcntl(fd, F_SETLKW, &fl);
    if (ret < 0) { close(fd); return -1; }
    return fd;
}

static void unlock_db(int fd)
{
    if (fd < 0) return;
    struct flock fl;
    memset(&fl, 0, sizeof(fl));
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    fcntl(fd, F_SETLKW, &fl);
    close(fd);
}

static json_object *load_db(void)
{
    int lockfd = lock_db(F_RDLCK);
    FILE *fp = fopen(DB_PATH, "r");
    if (!fp) { unlock_db(lockfd); return NULL; }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *buf = malloc(size + 1);
    if (!buf) { fclose(fp); unlock_db(lockfd); return NULL; }
    size = fread(buf, 1, size, fp);
    buf[size] = '\0';
    fclose(fp);
    unlock_db(lockfd);

    json_object *obj = json_tokener_parse(buf);
    free(buf);
    return obj;
}

static int save_db(json_object *root)
{
    int lockfd = lock_db(F_WRLCK);
    if (lockfd < 0) return -1;
    FILE *fp = fopen(DB_PATH, "w");
    if (!fp) { unlock_db(lockfd); return -1; }
    const char *str = json_object_to_json_string_ext(root,
        JSON_C_TO_STRING_PRETTY | JSON_C_TO_STRING_SPACED);
    fprintf(fp, "%s\n", str);
    fclose(fp);
    unlock_db(lockfd);
    return 0;
}

static void print_usage(const char *prog)
{
    fprintf(stderr, "Usage: %s <command> [args]\n\n", prog);
    fprintf(stderr, "Commands:\n");
    fprintf(stderr, "  groups              List all AP groups as JSON\n");
    fprintf(stderr, "  aps [--limit N]    List all APs as JSON\n");
    fprintf(stderr, "  alarms [--limit N]  List alarms as JSON\n");
    fprintf(stderr, "  firmware            List available firmware images as JSON\n");
    fprintf(stderr, "  stats               Database statistics\n");
    fprintf(stderr, "  ack <alarm_id>      Acknowledge an alarm\n");
    fprintf(stderr, "  ack-all             Acknowledge all alarms\n");
    fprintf(stderr, "  audit <user> <action> <rtype> <rid> <old> <new> <ip>  Write audit log\n");
}

/* Helper: get string from json object, returns "" if missing */
static const char *safe_get_str(json_object *obj, const char *key)
{
	json_object *jv;
	if (json_object_object_get_ex(obj, key, &jv) && jv)
		return json_object_get_string(jv);
	return "";
}

/* Helper: escape JSON string by replacing special characters */
static void json_escape(const char *src, char *dest, size_t dest_size)
{
	size_t i = 0, j = 0;
	while (src[i] && j < dest_size - 1) {
		switch (src[i]) {
		case '\\':
		case '"':
		case '\b':
		case '\f':
		case '\n':
		case '\r':
		case '\t':
			dest[j++] = '\\';
			switch (src[i]) {
			case '\b': dest[j++] = 'b'; break;
			case '\f': dest[j++] = 'f'; break;
			case '\n': dest[j++] = 'n'; break;
			case '\r': dest[j++] = 'r'; break;
			case '\t': dest[j++] = 't'; break;
			default:    dest[j++] = src[i]; break;
			}
			break;
		default:
			if (src[i] >= 0 && src[i] < 32) {
				// Control character, escape as \uXXXX
				snprintf(dest + j, dest_size - j, "\\u%04x", (unsigned char)src[i]);
				j += 6;
			} else {
				dest[j++] = src[i];
			}
			break;
		}
		i++;
	}
	dest[j] = '\0';
}

/* Helper: get int from json object, returns 0 if missing */
static int safe_get_int(json_object *obj, const char *key)
{
	json_object *jv;
	if (json_object_object_get_ex(obj, key, &jv) && jv)
		return json_object_get_int(jv);
	return 0;
}

/* Helper: get int as json string, returns "0" if missing
 * Uses static buffer — safe for single-threaded CLI use only. */
static const char *safe_get_int_str(json_object *obj, const char *key)
{
	static char int_buf[32];
	json_object *jv;
	if (json_object_object_get_ex(obj, key, &jv) && jv) {
		if (json_object_is_type(jv, json_type_int)) {
			snprintf(int_buf, sizeof(int_buf), "%d", json_object_get_int(jv));
			return int_buf;
		}
		/* If stored as string, return the string value directly */
		return json_object_get_string(jv);
	}
	return "0";
}

/* ---- groups ---- */
static int cmd_groups(void)
{
    json_object *root = load_db();
    if (!root) { fprintf(stderr, "Cannot open %s\n", DB_PATH); return 1; }

    json_object *groups;
    if (!json_object_object_get_ex(root, "ap_groups", &groups) ||
        !json_object_is_type(groups, json_type_array)) {
        printf("{\"groups\":[]}\n");
        json_object_put(root);
        return 0;
    }

    printf("{\"groups\":[");
    int len = json_object_array_length(groups);
    int i;
    for (i = 0; i < len; i++) {
        json_object *g = json_object_array_get_idx(groups, i);
        if (i > 0) printf(",");
        char name[512], desc[512], policy[512];
        json_escape(safe_get_str(g, "name"), name, sizeof(name));
        json_escape(safe_get_str(g, "description"), desc, sizeof(desc));
        json_escape(safe_get_str(g, "update_policy"), policy, sizeof(policy));
        printf("{\"id\":%s,\"name\":\"%s\",\"description\":\"%s\",\"policy\":\"%s\"}",
            safe_get_int_str(g, "id"),
            name,
            desc,
            policy);
    }
    printf("]}\n");
    json_object_put(root);
    return 0;
}

/* ---- aps ---- */
static int cmd_aps(int limit)
{
    json_object *root = load_db();
    if (!root) { fprintf(stderr, "Cannot open %s\n", DB_PATH); return 1; }

    json_object *nodes;
    if (!json_object_object_get_ex(root, "nodes", &nodes) ||
        !json_object_is_type(nodes, json_type_array)) {
        printf("{\"aps\":[]}\n");
        json_object_put(root);
        return 0;
    }

    printf("{\"aps\":[");
    int len = json_object_array_length(nodes);
    int count = 0;
    int i;
    for (i = 0; i < len && count < limit; i++) {
        json_object *n = json_object_array_get_idx(nodes, i);
        if (i > 0) printf(",");
        char mac[100], hostname[512], wan_ip[100], wifi_ssid[512], firmware[512], wifi_channel[50], wifi_encryption[100];
        json_escape(safe_get_str(n, "mac"), mac, sizeof(mac));
        json_escape(safe_get_str(n, "hostname"), hostname, sizeof(hostname));
        json_escape(safe_get_str(n, "wan_ip"), wan_ip, sizeof(wan_ip));
        json_escape(safe_get_str(n, "wifi_ssid"), wifi_ssid, sizeof(wifi_ssid));
        json_escape(safe_get_str(n, "firmware"), firmware, sizeof(firmware));
        json_escape(safe_get_str(n, "wifi_channel"), wifi_channel, sizeof(wifi_channel));
        json_escape(safe_get_str(n, "wifi_encryption"), wifi_encryption, sizeof(wifi_encryption));
        printf("{\"mac\":\"%s\",\"hostname\":\"%s\",\"wan_ip\":\"%s\","
               "\"wifi_ssid\":\"%s\",\"firmware\":\"%s\","
               "\"online_users\":%s,\"device_down\":%s,\"last_seen\":%s,\"group_id\":%s,"
               "\"ssid_count\":%s,\"wifi_channel\":\"%s\",\"wifi_encryption\":\"%s\"}",
            mac,
            hostname,
            wan_ip,
            wifi_ssid,
            firmware,
            safe_get_int_str(n, "online_user_num"),
            safe_get_int_str(n, "device_down"),
            safe_get_int_str(n, "last_seen"),
            safe_get_int_str(n, "group_id"),
            safe_get_int_str(n, "ssid_count"),
            wifi_channel,
            wifi_encryption);
        count++;
    }
    printf("]}\n");
    json_object_put(root);
    return 0;
}

/* ---- alarms ---- */
static int cmd_alarms(int limit)
{
    json_object *root = load_db();
    if (!root) { fprintf(stderr, "Cannot open %s\n", DB_PATH); return 1; }

    json_object *alarms;
    if (!json_object_object_get_ex(root, "alarm_events", &alarms) ||
        !json_object_is_type(alarms, json_type_array)) {
        printf("{\"alarms\":[]}\n");
        json_object_put(root);
        return 0;
    }

    const char *level_str[] = { "info", "warn", "error", "critical" };

    printf("{\"alarms\":[");
    int len = json_object_array_length(alarms);
    int count = 0;
    int i;
    for (i = len - 1; i >= 0 && count < limit; i--) {
        json_object *a = json_object_array_get_idx(alarms, i);
        if (i < len - 1) printf(",");
        int lvl = safe_get_int(a, "level");
        const char *lstr = (lvl >= 0 && lvl <= 3) ? level_str[lvl] : "unknown";
        char mac[100], message[1024], ts[100];
        json_escape(safe_get_str(a, "ap_mac"), mac, sizeof(mac));
        json_escape(safe_get_str(a, "message"), message, sizeof(message));
        json_escape(safe_get_str(a, "created_at"), ts, sizeof(ts));
        printf("{\"id\":%s,\"mac\":\"%s\",\"level\":\"%s\"," 
               "\"message\":\"%s\",\"ack\":%s,\"ts\":\"%s\"}",
            safe_get_int_str(a, "id"),
            mac,
            lstr,
            message,
            safe_get_int_str(a, "acknowledged"),
            ts);
        count++;
    }
    printf("]}\n");
    json_object_put(root);
    return 0;
}

/* ---- ack <id> ---- */
static int cmd_ack(const char *id_str)
{
    int alarm_id = atoi(id_str);

    json_object *root = load_db();
    if (!root) { fprintf(stderr, "Cannot open %s\n", DB_PATH); return 1; }

    json_object *alarms;
    if (!json_object_object_get_ex(root, "alarm_events", &alarms) ||
        !json_object_is_type(alarms, json_type_array)) {
        printf("{\"code\":1,\"message\":\"no alarms array\"}\n");
        json_object_put(root);
        return 1;
    }

    int len = json_object_array_length(alarms);
    for (int i = 0; i < len; i++) {
        json_object *a = json_object_array_get_idx(alarms, i);
        if (safe_get_int(a, "id") == alarm_id) {
            json_object_object_add(a, "acknowledged", json_object_new_int(1));
            json_object_object_add(a, "acknowledged_by", json_object_new_string("admin"));
            json_object_object_add(a, "acknowledged_at", json_object_new_string(""));
            save_db(root);
            printf("{\"code\":0,\"message\":\"ok\"}\n");
            json_object_put(root);
            return 0;
        }
    }

    printf("{\"code\":1,\"message\":\"alarm not found\"}\n");
    json_object_put(root);
    return 1;
}

/* ---- audit ---- */
static int cmd_audit(const char *user, const char *action,
    const char *rtype, const char *rid,
    const char *old_val, const char *new_val, const char *ip_addr)
{
    json_object *root = load_db();
    if (!root) { fprintf(stderr, "Cannot open %s\n", DB_PATH); return 1; }

    json_object *logs;
    if (!json_object_object_get_ex(root, "audit_logs", &logs)) {
        logs = json_object_new_array();
        json_object_object_add(root, "audit_logs", logs);
    }

    json_object *log = json_object_new_object();
    json_object_object_add(log, "id",            json_object_new_int(0));
    json_object_object_add(log, "user",           json_object_new_string(user    ? user    : "admin"));
    json_object_object_add(log, "action",         json_object_new_string(action  ? action  : ""));
    json_object_object_add(log, "resource_type",  json_object_new_string(rtype   ? rtype   : ""));
    json_object_object_add(log, "resource_id",    json_object_new_string(rid     ? rid     : ""));
    json_object_object_add(log, "old_value",      json_object_new_string(old_val ? old_val : ""));
    json_object_object_add(log, "new_value",      json_object_new_string(new_val ? new_val : ""));
    json_object_object_add(log, "ip_address",      json_object_new_string(ip_addr ? ip_addr : ""));
    json_object_object_add(log, "created_at",    json_object_new_string(""));

    /* Assign next id */
    int max_id = 0;
    int len = json_object_array_length(logs);
    for (int i = 0; i < len; i++) {
        json_object *l = json_object_array_get_idx(logs, i);
        int lid = safe_get_int(l, "id");
        if (lid > max_id) max_id = lid;
    }
    json_object_object_add(log, "id", json_object_new_int(max_id + 1));

    json_object_array_add(logs, log);
    save_db(root);
    printf("{\"code\":0}\n");
    json_object_put(root);
    return 0;
}

/* ---- ack-all ---- */
static int cmd_ack_all(void)
{
    json_object *root = load_db();
    if (!root) { fprintf(stderr, "Cannot open %s\n", DB_PATH); return 1; }

    json_object *alarms;
    if (!json_object_object_get_ex(root, "alarm_events", &alarms) ||
        !json_object_is_type(alarms, json_type_array)) {
        printf("{\"code\":0,\"acknowledged\":0}\n");
        json_object_put(root);
        return 0;
    }

    int len = json_object_array_length(alarms);
    int count = 0;
    int i;
    for (i = 0; i < len; i++) {
        json_object *a = json_object_array_get_idx(alarms, i);
        if (safe_get_int(a, "acknowledged") == 0) {
            json_object_object_add(a, "acknowledged", json_object_new_int(1));
            json_object_object_add(a, "acknowledged_by", json_object_new_string("admin"));
            json_object_object_add(a, "acknowledged_at", json_object_new_string(""));
            count++;
        }
    }

    if (count > 0)
        save_db(root);

    printf("{\"code\":0,\"acknowledged\":%d}\n", count);
    json_object_put(root);
    return 0;
}

/* ---- firmwares ---- */
static int cmd_firmwares(void)
{
    json_object *root = load_db();
    if (!root) { fprintf(stderr, "Cannot open %s\n", DB_PATH); return 1; }

    json_object *fws;
    if (!json_object_object_get_ex(root, "firmwares", &fws) ||
        !json_object_is_type(fws, json_type_array)) {
        printf("{\"firmwares\":[]}\n");
        json_object_put(root);
        return 0;
    }

    printf("{\"firmwares\":[");
    int len = json_object_array_length(fws);
    int i;
    for (i = len - 1; i >= 0; i--) {
        json_object *fw = json_object_array_get_idx(fws, i);
        if (i < len - 1) printf(",");
        char version[100], filename[512], sha256[100], uploaded_at[100];
        json_escape(safe_get_str(fw, "version"), version, sizeof(version));
        json_escape(safe_get_str(fw, "filename"), filename, sizeof(filename));
        json_escape(safe_get_str(fw, "sha256"), sha256, sizeof(sha256));
        json_escape(safe_get_str(fw, "uploaded_at"), uploaded_at, sizeof(uploaded_at));
        printf("{\"version\":\"%s\",\"filename\":\"%s\","
               "\"size\":%d,\"sha256\":\"%s\",\"uploaded_at\":\"%s\"}",
            version,
            filename,
            safe_get_int(fw, "file_size"),
            sha256,
            uploaded_at);
    }
    printf("]}\n");
    json_object_put(root);
    return 0;
}

/* ---- stats ---- */
static int cmd_stats(void)
{
    json_object *root = load_db();
    if (!root) { printf("{\"ap_total\":0,\"ap_online\":0,\"alarms\":0,\"groups\":0}\n"); return 1; }

    int ap_total = 0, ap_online = 0, alarms = 0, groups = 0;

    json_object *nodes;
    if (json_object_object_get_ex(root, "nodes", &nodes) &&
        json_object_is_type(nodes, json_type_array)) {
        ap_total = json_object_array_length(nodes);
        int nlen = ap_total;
        for (int i = 0; i < nlen; i++) {
            json_object *n = json_object_array_get_idx(nodes, i);
            if (safe_get_int(n, "device_down") == 0)
                ap_online++;
        }
    }

    json_object *ev;
    if (json_object_object_get_ex(root, "alarm_events", &ev) &&
        json_object_is_type(ev, json_type_array)) {
        int alen = json_object_array_length(ev);
        for (int i = 0; i < alen; i++) {
            json_object *a = json_object_array_get_idx(ev, i);
            if (safe_get_int(a, "acknowledged") == 0)
                alarms++;
        }
    }

    json_object *grp;
    if (json_object_object_get_ex(root, "ap_groups", &grp) &&
        json_object_is_type(grp, json_type_array)) {
        groups = json_object_array_length(grp);
    }

    printf("{\"ap_total\":%d,\"ap_online\":%d,\"alarms\":%d,\"groups\":%d}\n",
        ap_total, ap_online, alarms, groups);
    json_object_put(root);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2) { print_usage(argv[0]); return 1; }

    const char *cmd = argv[1];

    if (strcmp(cmd, "groups") == 0) {
        return cmd_groups();
    } else if (strcmp(cmd, "aps") == 0) {
        int limit = 500;
        if (argc >= 3 && strcmp(argv[2], "--limit") == 0 && argc >= 4)
            limit = atoi(argv[3]);
        return cmd_aps(limit);
    } else if (strcmp(cmd, "alarms") == 0) {
        int limit = 50;
        if (argc >= 3 && strcmp(argv[2], "--limit") == 0 && argc >= 4)
            limit = atoi(argv[3]);
        return cmd_alarms(limit);
    } else if (strcmp(cmd, "firmware") == 0) {
        return cmd_firmwares();
    } else if (strcmp(cmd, "audit") == 0) {
        return cmd_audit(
            argc >= 3 ? argv[2] : NULL,
            argc >= 4 ? argv[3] : NULL,
            argc >= 5 ? argv[4] : NULL,
            argc >= 6 ? argv[5] : NULL,
            argc >= 7 ? argv[6] : NULL,
            argc >= 8 ? argv[7] : NULL,
            argc >= 9 ? argv[8] : NULL);
    } else if (strcmp(cmd, "ack") == 0) {
        if (argc < 3) { fprintf(stderr, "ack requires alarm_id\n"); return 1; }
        return cmd_ack(argv[2]);
    } else if (strcmp(cmd, "ack-all") == 0) {
        return cmd_ack_all();
    } else if (strcmp(cmd, "stats") == 0) {
        return cmd_stats();
    } else {
        fprintf(stderr, "Unknown command: %s\n", cmd);
        print_usage(argv[0]);
        return 1;
    }
}
