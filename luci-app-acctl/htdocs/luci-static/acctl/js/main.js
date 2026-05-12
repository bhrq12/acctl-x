/**
 * AC Controller Main JavaScript Module
 * Provides UI interactions, status updates, and common utilities
 * Cross-browser compatible with IE8+
 */

var ACCTL = ACCTL || {};
ACCTL.UI = ACCTL.UI || {};

// Ensure compat module is loaded first
if (typeof ACCTL.compat === 'undefined') {
    throw new Error('Compatibility module not loaded');
}

var EventUtil = ACCTL.compat.EventUtil;
var DOMUtil = ACCTL.compat.DOMUtil;
var DateUtil = ACCTL.compat.DateUtil;

(function(ui, api) {
    'use strict';

    var STATUS_REFRESH_INTERVAL = 10000;
    var ALARM_REFRESH_INTERVAL = 15000;
    var AP_LIST_REFRESH_INTERVAL = 20000;

    var refreshTimers = {};
    var cache = {
        status: { data: null, timestamp: 0 },
        aps: { data: null, timestamp: 0 },
        alarms: { data: null, timestamp: 0 },
        groups: { data: null, timestamp: 0 }
    };
    var CACHE_TTL = 5000;

    function getCachedData(key) {
        var now = DateUtil.now();
        if (cache[key] && cache[key].timestamp + CACHE_TTL > now) {
            return cache[key].data;
        }
        return null;
    }

    function setCachedData(key, data) {
        cache[key] = {
            data: data,
            timestamp: DateUtil.now()
        };
    }

    function clearCache() {
        cache = {
            status: { data: null, timestamp: 0 },
            aps: { data: null, timestamp: 0 },
            alarms: { data: null, timestamp: 0 },
            groups: { data: null, timestamp: 0 }
        };
    }

    ui.init = function() {
        ui.setupStatusWidget();
        ui.setupAPList();
        ui.setupAlarmPanel();
        ui.setupActionButtons();
        ui.setupConfirmDialogs();
        ui.setupAutoRefresh();
    };

    ui.setupStatusWidget = function() {
        var widget = DOMUtil.getById('ac_status_widget');
        if (!widget) return;

        var updateStatus = function() {
            var cached = getCachedData('status');
            if (cached) {
                ui.updateStatusDisplay(cached);
            }

            api.getStatus(function(err, data) {
                if (err || !data) return;

                setCachedData('status', data);
                ui.updateStatusDisplay(data);
            });
        };

        updateStatus();
        refreshTimers.status = setInterval(updateStatus, STATUS_REFRESH_INTERVAL);

        EventUtil.addListener(widget, 'click', function() {
            window.location.href = '/cgi-bin/luci/admin/network/acctl/ap_list';
        });
    };

    ui.updateStatusDisplay = function(data) {
        var dot = DOMUtil.getById('ac_status_dot');
        var online = DOMUtil.getById('ac_online');
        var total = DOMUtil.getById('ac_total');
        var alarms = DOMUtil.getById('ac_alarms');
        var alarmContainer = DOMUtil.getById('ac_alarm_container');

        if (online) online.textContent = data.ap_online || 0;
        if (total) total.textContent = data.ap_total || 0;

        var alarmCount = data.alarm_count || 0;
        if (alarms) alarms.textContent = alarmCount;

        if (alarmContainer) {
            alarmContainer.style.display = alarmCount > 0 ? 'inline' : 'none';
            alarmContainer.style.color = alarmCount > 0 ? '#dc3545' : '#ffc107';
        }

        if (dot) {
            if (data.running) {
                dot.innerHTML = '&#x25CF;';
                dot.style.color = '#28a745';
            } else {
                dot.innerHTML = '&#x25CB;';
                dot.style.color = '#dc3545';
            }
        }
    };

    ui.setupAPList = function() {
        var checkboxes = DOMUtil.querySelectorAll('input[name="cbid.acctl.ap_list.*.enabled"]');
        var selectAll = DOMUtil.getById('select_all');

        if (selectAll) {
            EventUtil.addListener(selectAll, 'change', function(e) {
                var event = EventUtil.getEvent(e);
                var target = EventUtil.getTarget(event);
                checkboxes.forEach(function(cb) {
                    cb.checked = target.checked;
                });
                ui.updateSelectedCount();
            });
        }

        checkboxes.forEach(function(cb) {
            EventUtil.addListener(cb, 'change', ui.updateSelectedCount);
        });

        var rows = DOMUtil.querySelectorAll('table.cbi-section-table tbody tr');
        rows.forEach(function(row) {
            EventUtil.addListener(row, 'mouseenter', function() {
                this.style.backgroundColor = '#f8f9fa';
            });
            EventUtil.addListener(row, 'mouseleave', function() {
                this.style.backgroundColor = '';
            });
        });
    };

    ui.updateSelectedCount = function() {
        var checkboxes = DOMUtil.querySelectorAll('input[name="cbid.acctl.ap_list.*.enabled"]');
        var selected = Array.prototype.filter.call(checkboxes, function(cb) {
            return cb.checked;
        }).length;

        var countEl = DOMUtil.getById('selected_count');
        if (countEl) {
            countEl.textContent = selected;
        }

        var actionButtons = DOMUtil.querySelectorAll('.ap-action-btn');
        actionButtons.forEach(function(btn) {
            btn.disabled = selected === 0;
        });
    };

    ui.setupAlarmPanel = function() {
        var ackAllBtn = DOMUtil.getById('ack_all_alarms');
        if (ackAllBtn) {
            EventUtil.addListener(ackAllBtn, 'click', function() {
                ui.showConfirmDialog(
                    '确认操作',
                    '确定要确认所有告警吗？',
                    function() {
                        api.acknowledgeAlarms('all', function(err, data) {
                            if (!err) {
                                ui.showToast('告警已全部确认');
                                clearCache();
                                location.reload();
                            } else {
                                ui.showToast('操作失败', 'error');
                            }
                        });
                    }
                );
            });
        }

        var ackButtons = DOMUtil.querySelectorAll('.ack-alarm-btn');
        ackButtons.forEach(function(btn) {
            EventUtil.addListener(btn, 'click', function() {
                var alarmId = DOMUtil.getDataAttr(btn, 'alarmId');
                api.acknowledgeAlarms(alarmId, function(err, data) {
                    if (!err) {
                        ui.showToast('告警已确认');
                        clearCache();
                        location.reload();
                    } else {
                        ui.showToast('操作失败', 'error');
                    }
                });
            });
        });
    };

    ui.setupActionButtons = function() {
        var rebootBtn = DOMUtil.getById('reboot_selected');
        if (rebootBtn) {
            EventUtil.addListener(rebootBtn, 'click', function() {
                ui.executeAPAction('reboot', '重启');
            });
        }

        var configBtn = DOMUtil.getById('config_selected');
        if (configBtn) {
            EventUtil.addListener(configBtn, 'click', function() {
                ui.executeAPAction('config', '配置推送');
            });
        }

        var upgradeBtn = DOMUtil.getById('upgrade_selected');
        if (upgradeBtn) {
            EventUtil.addListener(upgradeBtn, 'click', function() {
                ui.executeAPAction('upgrade', '固件升级');
            });
        }
    };

    ui.executeAPAction = function(action, actionName) {
        var checkboxes = DOMUtil.querySelectorAll('input[name="cbid.acctl.ap_list.*.enabled"]');
        var selectedMacs = [];

        checkboxes.forEach(function(cb) {
            if (cb.checked) {
                var mac = DOMUtil.getDataAttr(cb, 'mac') || cb.value;
                if (mac) selectedMacs.push(mac);
            }
        });

        if (selectedMacs.length === 0) {
            ui.showToast('请先选择AP', 'warning');
            return;
        }

        ui.showConfirmDialog(
            '确认' + actionName,
            '确定要对选中的 ' + selectedMacs.length + ' 个AP执行' + actionName + '操作吗？',
            function() {
                api.apAction(action, selectedMacs.join(','), function(err, data) {
                    if (!err) {
                        ui.showToast(actionName + '命令已提交');
                        clearCache();
                    } else {
                        ui.showToast('操作失败', 'error');
                    }
                });
            }
        );
    };

    ui.setupConfirmDialogs = function() {
        var startBtn = DOMUtil.getById('btn_start');
        var stopBtn = DOMUtil.getById('btn_stop');
        var restartBtn = DOMUtil.getById('btn_restart');

        if (startBtn) {
            EventUtil.addListener(startBtn, 'click', function() {
                ui.showConfirmDialog('确认启动', '确定要启动AC控制器吗？', function() {
                    api.controllerAction('start', ui.handleControllerResponse);
                });
            });
        }

        if (stopBtn) {
            EventUtil.addListener(stopBtn, 'click', function() {
                ui.showConfirmDialog('确认停止', '确定要停止AC控制器吗？', function() {
                    api.controllerAction('stop', ui.handleControllerResponse);
                });
            });
        }

        if (restartBtn) {
            EventUtil.addListener(restartBtn, 'click', function() {
                ui.showConfirmDialog('确认重启', '确定要重启AC控制器吗？', function() {
                    api.restartController(ui.handleControllerResponse);
                });
            });
        }
    };

    ui.handleControllerResponse = function(err, data) {
        if (!err && data) {
            ui.showToast(data.message || '操作成功');
            clearCache();
            setTimeout(function() {
                api.getStatus(function(err, status) {
                    if (!err) ui.updateStatusDisplay(status);
                });
            }, 2000);
        } else {
            ui.showToast(err ? err.message : '操作失败', 'error');
        }
    };

    ui.setupAutoRefresh = function() {
        if (DOMUtil.getById('alarm-panel')) {
            var updateAlarms = function() {
                api.getAlarms(function(err, data) {
                    if (!err && data) {
                        setCachedData('alarms', data);
                    }
                });
            };
            refreshTimers.alarms = setInterval(updateAlarms, ALARM_REFRESH_INTERVAL);
        }

        if (DOMUtil.getById('ap-list-container')) {
            var updateAPs = function() {
                api.getAPs(function(err, data) {
                    if (!err && data) {
                        setCachedData('aps', data);
                    }
                });
            };
            refreshTimers.aps = setInterval(updateAPs, AP_LIST_REFRESH_INTERVAL);
        }
    };

    ui.showConfirmDialog = function(title, message, onConfirm) {
        var dialog = DOMUtil.createElement('div', { className: 'acctl-confirm-dialog' });
        dialog.innerHTML = [
            '<div class="acctl-dialog-overlay"></div>',
            '<div class="acctl-dialog-content">',
            '<h3>' + title + '</h3>',
            '<p>' + message + '</p>',
            '<div class="acctl-dialog-buttons">',
            '<button class="btn-cancel">取消</button>',
            '<button class="btn-confirm">确定</button>',
            '</div>',
            '</div>'
        ].join('');

        document.body.appendChild(dialog);

        var overlay = DOMUtil.querySelector('.acctl-dialog-overlay', dialog);
        var btnCancel = DOMUtil.querySelector('.btn-cancel', dialog);
        var btnConfirm = DOMUtil.querySelector('.btn-confirm', dialog);

        var closeDialog = function() {
            if (document.body.contains(dialog)) {
                document.body.removeChild(dialog);
            }
        };

        EventUtil.addListener(overlay, 'click', closeDialog);
        EventUtil.addListener(btnCancel, 'click', closeDialog);
        EventUtil.addListener(btnConfirm, 'click', function() {
            closeDialog();
            if (typeof onConfirm === 'function') {
                onConfirm();
            }
        });
    };

    ui.showToast = function(message, type) {
        type = type || 'success';

        var toast = DOMUtil.createElement('div', { 
            className: 'acctl-toast acctl-toast-' + type,
            textContent: message
        });

        document.body.appendChild(toast);

        setTimeout(function() {
            if (document.body.contains(toast)) {
                document.body.removeChild(toast);
            }
        }, 3000);
    };

    ui.formatMAC = function(mac) {
        if (!mac) return '';
        var clean = mac.replace(/[^a-fA-F0-9]/g, '');
        var parts = clean.match(/.{1,2}/g);
        if (!parts) return '';
        return parts.join(':').toUpperCase();
    };

    ui.formatTime = function(timestamp) {
        if (!timestamp) return 'N/A';
        var date = typeof timestamp === 'number' ? new Date(timestamp * 1000) : new Date(timestamp);
        return date.toLocaleString();
    };

    ui.getRelativeTime = function(timestamp) {
        if (!timestamp) return 'N/A';
        var now = Math.floor(DateUtil.now() / 1000);
        var diff = now - timestamp;

        if (diff < 60) return '刚刚';
        if (diff < 3600) return Math.floor(diff / 60) + ' 分钟前';
        if (diff < 86400) return Math.floor(diff / 3600) + ' 小时前';
        if (diff < 604800) return Math.floor(diff / 86400) + ' 天前';
        return ui.formatTime(timestamp);
    };

    ui.destroy = function() {
        for (var key in refreshTimers) {
            if (refreshTimers.hasOwnProperty(key)) {
                clearInterval(refreshTimers[key]);
            }
        }
        clearCache();
    };

})(ACCTL.UI, ACCTL.API);

// Initialize on DOM ready
(function() {
    if (document.readyState === 'complete') {
        ACCTL.UI.init();
    } else {
        EventUtil.addListener(document, 'DOMContentLoaded', ACCTL.UI.init);
    }
})();

// Cleanup on unload
EventUtil.addListener(window, 'beforeunload', function() {
    if (ACCTL.UI.destroy) {
        ACCTL.UI.destroy();
    }
});