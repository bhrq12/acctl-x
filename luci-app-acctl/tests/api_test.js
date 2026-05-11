/**
 * AC Controller - JavaScript Unit Tests
 * Run with: node tests/api_test.js
 */

const ACCTL = {
    API: {},
    UI: {}
};

(function(api) {
    'use strict';

    const API_BASE = '/cgi-bin/luci/admin/network/acctl/api';
    const REQUEST_TIMEOUT = 10000;

    let lastRequest = null;
    let mockResponse = null;
    let mockError = null;

    function request(method, endpoint, params, callback) {
        if (typeof params === 'function') {
            callback = params;
            params = null;
        }

        lastRequest = {
            method: method,
            endpoint: endpoint,
            params: params
        };

        if (mockError) {
            callback && callback(new Error(mockError), null);
            return;
        }

        if (mockResponse) {
            callback && callback(null, mockResponse);
        }
    }

    api.getStatus = function(callback) {
        request('GET', '/status', callback);
    };

    api.getAPs = function(params, callback) {
        if (typeof params === 'function') {
            callback = params;
            params = null;
        }
        request('GET', '/aps', params, callback);
    };

    api.getAPDetails = function(mac, callback) {
        request('GET', '/aps', { mac: mac }, callback);
    };

    api.apAction = function(action, macs, callback) {
        request('POST', '/aps_action', {
            action: action,
            macs: macs
        }, callback);
    };

    api.getAlarms = function(params, callback) {
        if (typeof params === 'function') {
            callback = params;
            params = null;
        }
        request('GET', '/alarms', params, callback);
    };

    api.acknowledgeAlarms = function(ids, callback) {
        request('POST', '/alarms_ack', { ids: ids }, callback);
    };

    api.getGroups = function(callback) {
        request('GET', '/groups', callback);
    };

    api.getFirmwares = function(callback) {
        request('GET', '/firmwares', callback);
    };

    api.executeCommand = function(mac, cmd, callback) {
        request('POST', '/cmd', { mac: mac, cmd: cmd }, callback);
    };

    api.controllerAction = function(action, callback) {
        request('POST', '/action', { action: action }, callback);
    };

    api.restartController = function(callback) {
        request('POST', '/restart', callback);
    };

    api.switchMode = function(mode, callback) {
        request('POST', '/switch_mode', { mode: mode }, callback);
    };

    api.setMockResponse = function(response) {
        mockResponse = response;
    };

    api.setMockError = function(error) {
        mockError = error;
    };

    api.clearMock = function() {
        mockResponse = null;
        mockError = null;
        lastRequest = null;
    };

    api.getLastRequest = function() {
        return lastRequest;
    };

    api.API_BASE = API_BASE;
    api.REQUEST_TIMEOUT = REQUEST_TIMEOUT;

})(ACCTL.API);

(function(ui) {
    'use strict';

    let toasts = [];
    let dialogs = [];

    ui.showToast = function(message, type) {
        type = type || 'success';
        toasts.push({ message: message, type: type });
    };

    ui.showConfirmDialog = function(title, message, onConfirm) {
        dialogs.push({ title: title, message: message, onConfirm: onConfirm });
    };

    ui.formatMAC = function(mac) {
        if (!mac) return '';
        var clean = mac.replace(/[^a-fA-F0-9]/g, '');
        return clean.match(/.{1,2}/g).join(':').toUpperCase();
    };

    ui.formatTime = function(timestamp) {
        if (!timestamp) return 'N/A';
        var date = typeof timestamp === 'number' ? new Date(timestamp * 1000) : new Date(timestamp);
        return date.toLocaleString();
    };

    ui.getRelativeTime = function(timestamp) {
        if (!timestamp) return 'N/A';
        var now = Math.floor(Date.now() / 1000);
        var diff = now - timestamp;

        if (diff < 60) return '\u5219\u521A';
        if (diff < 3600) return Math.floor(diff / 60) + ' \u5206\u949F\u524D';
        if (diff < 86400) return Math.floor(diff / 3600) + ' \u5C0F\u65F6\u524D';
        if (diff < 604800) return Math.floor(diff / 86400) + ' \u5929\u524D';
        return ui.formatTime(timestamp);
    };

    ui.getToasts = function() {
        return toasts;
    };

    ui.getDialogs = function() {
        return dialogs;
    };

    ui.clearToasts = function() {
        toasts = [];
    };

    ui.clearDialogs = function() {
        dialogs = [];
    };

})(ACCTL.UI);

const TestSuite = {
    passed: 0,
    failed: 0,

    run: function(name, tests) {
        console.log('\n========================================');
        console.log('Running: ' + name);
        console.log('========================================');

        for (const testName in tests) {
            try {
                tests[testName]();
                console.log('[PASS] ' + testName);
                this.passed++;
            } catch (e) {
                console.log('[FAIL] ' + testName);
                console.log('       Error: ' + e.message);
                this.failed++;
            }
        }

        console.log('----------------------------------------');
        console.log('Results: ' + this.passed + ' passed, ' + this.failed + ' failed');
        console.log('========================================\n');

        return this.failed === 0;
    }
};

function assertEqual(expected, actual, msg) {
    if (expected !== actual) {
        throw new Error((msg || 'Assertion failed') + ': expected ' +
            JSON.stringify(expected) + ', got ' + JSON.stringify(actual));
    }
}

function assertTrue(val, msg) {
    if (!val) {
        throw new Error(msg || 'Assertion failed: expected true');
    }
}

function assertFalse(val, msg) {
    if (val) {
        throw new Error(msg || 'Assertion failed: expected false');
    }
}

function assertContains(str, substr, msg) {
    if (!str || str.indexOf(substr) === -1) {
        throw new Error(msg || 'Assertion failed: string does not contain substring');
    }
}

// Tests
const tests = {
    'API_BASE constant': function() {
        assertEqual('/cgi-bin/luci/admin/network/acctl/api', ACCTL.API.API_BASE);
    },

    'REQUEST_TIMEOUT constant': function() {
        assertEqual(10000, ACCTL.API.REQUEST_TIMEOUT);
    },

    'getStatus request': function() {
        ACCTL.API.clearMock();
        ACCTL.API.setMockResponse({ running: true, ap_online: 5 });
        ACCTL.API.getStatus(function(err, data) {
            assertTrue(err === null, 'Should not have error');
        });
        const req = ACCTL.API.getLastRequest();
        assertEqual('GET', req.method);
        assertEqual('/status', req.endpoint);
    },

    'getAPs request': function() {
        ACCTL.API.clearMock();
        ACCTL.API.setMockResponse({ aps: [], count: 0 });
        ACCTL.API.getAPs(function(err, data) {
            assertTrue(err === null, 'Should not have error');
        });
        const req = ACCTL.API.getLastRequest();
        assertEqual('GET', req.method);
        assertEqual('/aps', req.endpoint);
    },

    'getAPDetails request': function() {
        ACCTL.API.clearMock();
        ACCTL.API.setMockResponse({ mac: 'AA:BB:CC:DD:EE:FF' });
        ACCTL.API.getAPDetails('AA:BB:CC:DD:EE:FF', function(err, data) {});
        const req = ACCTL.API.getLastRequest();
        assertEqual('GET', req.method);
        assertEqual('/aps', req.endpoint);
        assertEqual('AA:BB:CC:DD:EE:FF', req.params.mac);
    },

    'apAction request': function() {
        ACCTL.API.clearMock();
        ACCTL.API.setMockResponse({ code: 0, message: 'Success' });
        ACCTL.API.apAction('reboot', 'AA:BB:CC:DD:EE:FF', function(err, data) {});
        const req = ACCTL.API.getLastRequest();
        assertEqual('POST', req.method);
        assertEqual('/aps_action', req.endpoint);
        assertEqual('reboot', req.params.action);
        assertEqual('AA:BB:CC:DD:EE:FF', req.params.macs);
    },

    'getAlarms request': function() {
        ACCTL.API.clearMock();
        ACCTL.API.setMockResponse({ alarms: [], count: 0 });
        ACCTL.API.getAlarms(function(err, data) {});
        const req = ACCTL.API.getLastRequest();
        assertEqual('GET', req.method);
        assertEqual('/alarms', req.endpoint);
    },

    'acknowledgeAlarms request': function() {
        ACCTL.API.clearMock();
        ACCTL.API.setMockResponse({ code: 0, acknowledged: 1 });
        ACCTL.API.acknowledgeAlarms('1,2,3', function(err, data) {});
        const req = ACCTL.API.getLastRequest();
        assertEqual('POST', req.method);
        assertEqual('/alarms_ack', req.endpoint);
        assertEqual('1,2,3', req.params.ids);
    },

    'getGroups request': function() {
        ACCTL.API.clearMock();
        ACCTL.API.setMockResponse({ groups: [] });
        ACCTL.API.getGroups(function(err, data) {});
        const req = ACCTL.API.getLastRequest();
        assertEqual('GET', req.method);
        assertEqual('/groups', req.endpoint);
    },

    'getFirmwares request': function() {
        ACCTL.API.clearMock();
        ACCTL.API.setMockResponse({ firmwares: [] });
        ACCTL.API.getFirmwares(function(err, data) {});
        const req = ACCTL.API.getLastRequest();
        assertEqual('GET', req.method);
        assertEqual('/firmwares', req.endpoint);
    },

    'executeCommand request': function() {
        ACCTL.API.clearMock();
        ACCTL.API.setMockResponse({ code: 0, message: 'Success' });
        ACCTL.API.executeCommand('AA:BB:CC:DD:EE:FF', 'uptime', function(err, data) {});
        const req = ACCTL.API.getLastRequest();
        assertEqual('POST', req.method);
        assertEqual('/cmd', req.endpoint);
        assertEqual('AA:BB:CC:DD:EE:FF', req.params.mac);
        assertEqual('uptime', req.params.cmd);
    },

    'controllerAction request': function() {
        ACCTL.API.clearMock();
        ACCTL.API.setMockResponse({ code: 0, message: 'Success' });
        ACCTL.API.controllerAction('restart', function(err, data) {});
        const req = ACCTL.API.getLastRequest();
        assertEqual('POST', req.method);
        assertEqual('/action', req.endpoint);
        assertEqual('restart', req.params.action);
    },

    'restartController request': function() {
        ACCTL.API.clearMock();
        ACCTL.API.setMockResponse({ code: 0, message: 'Restarting' });
        ACCTL.API.restartController(function(err, data) {});
        const req = ACCTL.API.getLastRequest();
        assertEqual('POST', req.method);
        assertEqual('/restart', req.endpoint);
    },

    'switchMode request': function() {
        ACCTL.API.clearMock();
        ACCTL.API.setMockResponse({ code: 0, message: 'Mode switched' });
        ACCTL.API.switchMode('ac', function(err, data) {});
        const req = ACCTL.API.getLastRequest();
        assertEqual('POST', req.method);
        assertEqual('/switch_mode', req.endpoint);
        assertEqual('ac', req.params.mode);
    },

    'formatMAC valid': function() {
        const mac = ACCTL.UI.formatMAC('aabbccddeeff');
        assertEqual('AA:BB:CC:DD:EE:FF', mac);
    },

    'formatMAC empty': function() {
        const mac = ACCTL.UI.formatMAC(null);
        assertEqual('', mac);
    },

    'formatTime valid': function() {
        const time = ACCTL.UI.formatTime(1714304000);
        assertTrue(time !== 'N/A', 'Should format valid timestamp');
    },

    'formatTime empty': function() {
        const time = ACCTL.UI.formatTime(null);
        assertEqual('N/A', time);
    },

    'getRelativeTime recent': function() {
        const now = Math.floor(Date.now() / 1000);
        const time = ACCTL.UI.getRelativeTime(now);
        assertEqual('\u5219\u521A', time);
    },

    'getRelativeTime minutes': function() {
        const now = Math.floor(Date.now() / 1000);
        const time = ACCTL.UI.getRelativeTime(now - 300);
        assertContains(time, '\u5206\u949F\u524D', 'Should contain minutes');
    },

    'getRelativeTime empty': function() {
        const time = ACCTL.UI.getRelativeTime(null);
        assertEqual('N/A', time);
    },

    'showToast': function() {
        ACCTL.UI.clearToasts();
        ACCTL.UI.showToast('Test message', 'success');
        const toasts = ACCTL.UI.getToasts();
        assertEqual(1, toasts.length);
        assertEqual('Test message', toasts[0].message);
        assertEqual('success', toasts[0].type);
    },

    'showConfirmDialog': function() {
        ACCTL.UI.clearDialogs();
        ACCTL.UI.showConfirmDialog('Title', 'Message', function() {});
        const dialogs = ACCTL.UI.getDialogs();
        assertEqual(1, dialogs.length);
        assertEqual('Title', dialogs[0].title);
        assertEqual('Message', dialogs[0].message);
    }
};

// Run tests
const success = TestSuite.run('AC Controller JavaScript Tests', tests);

// Exit with appropriate code
process.exit(success ? 0 : 1);
