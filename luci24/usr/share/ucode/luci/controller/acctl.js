// LuCI - AC Controller JavaScript Controller
// Compatible with OpenWrt 24.x (ucode-based LuCI)

'use strict';

import { curent } from 'uci';
import { exec, call } from 'ubus';

return class ACctlController extends LuCI.controller {
    constructor() {
        super();
        this.acctl_cli = '/usr/bin/acctl-cli';
    }

    getCLI(cmd) {
        try {
            const f = exec(this.acctl_cli + ' ' + cmd + ' 2>/dev/null');
            if (!f || f.stdout === '') return null;
            return JSON.parse(f.stdout);
        } catch (e) {
            return null;
        }
    }

    isRunning() {
        try {
            exec('pgrep -x acser > /dev/null 2>&1');
            return call('service', 'check', { name: 'acctl' }) === 0;
        } catch (e) {
            return false;
        }
    }

    registerI18n() {
        return _('AC Controller');
    }

    index() {
        super.entry({
            title: _('AC Controller'),
            path: '/admin/network/acctl',
            icon: 'signal',
        }, super.chain(
            super.cart('admin/network/acctl/general'),
            super.cart('admin/network/acctl/ap_list'),
            super.cart('admin/network/acctl/groups'),
            super.cart('admin/network/acctl/alarms'),
            super.cart('admin/network/acctl/firmware'),
            super.cart('admin/network/acctl/system')
        ));

        super.entry({
            title: _('AC Controller Status'),
            path: '/admin/services/acctl',
            icon: 'signal',
        }, super.cart('admin/services/acctl/status'));
    }

    apiStatus() {
        const stats = this.getCLI('stats') || {};
        const running = this.isRunning();
        const response = {
            running: running,
            ap_online: parseInt(stats.ap_online) || 0,
            ap_total: parseInt(stats.ap_total) || 0,
            alarm_count: parseInt(stats.alarms) || 0,
            pool_left: 0,
            pool_total: 0,
            timestamp: Math.floor(Date.now() / 1000)
        };
        http.write_json(response);
    }

    apiAps() {
        const mac = http.formvalue('mac');
        const result = { aps: [], count: 0 };

        if (mac && mac !== '') {
            const data = this.getCLI('aps --limit 500');
            if (data && data.aps) {
                for (const ap of data.aps) {
                    if (ap.mac === mac) {
                        result.mac = ap.mac || '';
                        result.hostname = ap.hostname || '';
                        result.wan_ip = ap.wan_ip || '';
                        result.wifi_ssid = ap.wifi_ssid || '';
                        result.firmware = ap.firmware || '';
                        result.online_users = parseInt(ap.online_users) || 0;
                        result.online = parseInt(ap.device_down) === 0;
                        result.last_seen = parseInt(ap.last_seen) || 0;
                        result.group_id = parseInt(ap.group_id) || 0;
                        break;
                    }
                }
            }
        } else {
            const data = this.getCLI('aps --limit 500');
            if (data && data.aps) {
                for (const ap of data.aps) {
                    result.aps.push({
                        mac: ap.mac || '',
                        hostname: ap.hostname || '',
                        wan_ip: ap.wan_ip || '',
                        wifi_ssid: ap.wifi_ssid || '',
                        firmware: ap.firmware || '',
                        online_users: parseInt(ap.online_users) || 0,
                        online: parseInt(ap.device_down) === 0,
                        last_seen: parseInt(ap.last_seen) || 0,
                        group_id: parseInt(ap.group_id) || 0
                    });
                    result.count++;
                }
            }
        }
        http.write_json(result);
    }

    apiApsAction() {
        const action = http.formvalue('action');
        const macs = http.formvalue('macs') || '';
        const result = { code: 0, message: '', affected: 0 };

        if (action === '' || macs === '') {
            result.code = 400;
            result.message = 'action and macs required';
            http.write_json(result);
            return;
        }

        const valid_actions = { reboot: true, config: true, upgrade: true };
        if (!valid_actions[action]) {
            result.code = 400;
            result.message = 'Invalid action';
            http.write_json(result);
            return;
        }

        const safe_macs = macs.replace(/[^%x:, ]/g, '');
        this.getCLI(`audit admin ${action} ap_batch '${safe_macs.substring(0, 50)}' '' '' ''`);

        if (action === 'reboot') result.message = 'Reboot command queued for APs';
        else if (action === 'config') result.message = 'Configuration push queued';
        else if (action === 'upgrade') result.message = 'Firmware upgrade queued';

        http.write_json(result);
    }

    apiAlarms() {
        const limit = parseInt(http.formvalue('limit')) || 50;
        const data = this.getCLI('alarms --limit ' + limit) || { alarms: [] };
        const alarms = [];

        for (const a of (data.alarms || [])) {
            let level = 0;
            if (a.level === 'warn') level = 1;
            else if (a.level === 'error') level = 2;
            else if (a.level === 'critical') level = 3;

            alarms.push({
                id: parseInt(a.id) || 0,
                ap_mac: a.mac || '',
                level: a.level || 'info',
                level_num: level,
                message: a.message || '',
                acknowledged: a.ack == 1 || a.ack === true,
                created_at: a.ts || ''
            });
        }
        http.write_json({ alarms: alarms, count: alarms.length });
    }

    apiAlarmsAck() {
        const ids = http.formvalue('ids') || '';
        const result = { code: 0, acknowledged: 0 };

        if (ids === 'all') {
            this.getCLI('ack-all');
            result.acknowledged = -1;
        } else {
            const idList = ids.match(/\d+/g) || [];
            for (const id of idList) {
                this.getCLI('ack ' + id);
                result.acknowledged++;
            }
        }
        http.write_json(result);
    }

    apiGroups() {
        const data = this.getCLI('groups') || { groups: [] };
        const groups = [];

        const aps_data = this.getCLI('aps --limit 500') || { aps: [] };
        const ap_count = {};
        for (const ap of (aps_data.aps || [])) {
            const gid = parseInt(ap.group_id) || 0;
            ap_count[gid] = (ap_count[gid] || 0) + 1;
        }

        for (const grp of (data.groups || [])) {
            groups.push({
                id: parseInt(grp.id) || 0,
                name: grp.name || '',
                description: grp.description || '',
                policy: grp.policy || 'manual',
                ap_count: ap_count[parseInt(grp.id) || 0] || 0
            });
        }
        http.write_json({ groups: groups });
    }

    apiFirmwares() {
        const data = this.getCLI('firmware') || { firmwares: [] };
        const firmwares = [];

        for (const fw of (data.firmwares || [])) {
            firmwares.push({
                version: fw.version || '',
                filename: fw.filename || '',
                size: parseInt(fw.size) || 0,
                sha256: fw.sha256 || '',
                uploaded_at: fw.uploaded_at || ''
            });
        }
        http.write_json({ firmwares: firmwares });
    }

    apiCmd() {
        const mac = http.formvalue('mac');
        const cmd = http.formvalue('cmd');
        const result = { code: 0, message: '' };

        if (!mac || !cmd || mac === '' || cmd === '') {
            result.code = 400;
            result.message = 'mac and cmd are required';
            http.write_json(result);
            return;
        }

        const allowed = {
            'reboot': true,
            'uptime': true,
            'ifconfig': true,
            'iwconfig': true,
            'wifi': true,
            'cat /proc/uptime': true,
            'cat /proc/loadavg': true,
            'cat /tmp/ap_status': true
        };

        const dangerous = [';', '|', '`', '$(', '&', '&&', '||', '>', '<', '>>',
            '/bin/', '/etc/', '/usr/', '/var/', '/dev/', '/proc/', '/root/', '../',
            'nohup', 'screen', 'tmux', 'wget', 'curl', 'python', 'perl', 'ruby',
            'php', 'bash', 'sh -c', 'exec', 'eval', 'chmod', 'chown', 'rm -rf',
            'mkfs', 'dd', 'fdisk', 'mount', 'umount', 'passwd', 'su', 'sudo'];

        for (const p of dangerous) {
            if (cmd.indexOf(p) !== -1) {
                result.code = 403;
                result.message = 'Command contains forbidden patterns';
                http.write_json(result);
                return;
            }
        }

        if (!allowed[cmd]) {
            result.code = 403;
            result.message = 'Command not in whitelist';
            http.write_json(result);
            return;
        }

        const safe_mac = mac.replace(/[^%x:]/g, '').substring(0, 20);
        this.getCLI(`audit admin EXEC ap '${safe_mac}' '' '${cmd}' ''`);

        result.message = 'Command queued successfully';
        http.write_json(result);
    }

    apiRestart() {
        const result = { code: 0, message: '' };
        try {
            exec('/etc/init.d/acctl restart > /dev/null 2>&1');
            result.message = this.isRunning() ? 'AC Controller restarting...' : 'AC Controller starting...';
        } catch (e) {
            result.code = 500;
            result.message = 'Failed to restart service';
        }
        http.write_json(result);
    }

    apiAction() {
        const action = http.formvalue('action');
        const result = { code: 0, message: '' };

        try {
            if (action === 'start') {
                exec('/etc/init.d/acctl start > /dev/null 2>&1');
                result.message = 'AC Controller started';
            } else if (action === 'stop') {
                exec('/etc/init.d/acctl stop > /dev/null 2>&1');
                result.message = 'AC Controller stopped';
            } else if (action === 'restart') {
                exec('/etc/init.d/acctl restart > /dev/null 2>&1');
                result.message = 'AC Controller restarted';
            } else {
                result.code = 400;
                result.message = 'Invalid action';
            }
        } catch (e) {
            result.code = 500;
            result.message = 'Failed to execute action';
        }
        http.write_json(result);
    }

    apiSwitchMode() {
        const mode = http.formvalue('mode');
        const result = { code: 0, message: '' };

        try {
            if (mode === 'ac' || mode === 'ap') {
                const uci = curent();
                uci.set('acctl', 'acctl', 'mode', mode);
                uci.commit('acctl');
                exec('/etc/init.d/acctl restart > /dev/null 2>&1');
                result.message = 'Mode switched to ' + (mode === 'ac' ? 'AC' : 'AP') + ' mode';
            } else {
                result.code = 400;
                result.message = 'Invalid mode';
            }
        } catch (e) {
            result.code = 500;
            result.message = 'Failed to switch mode';
        }
        http.write_json(result);
    }
});