/**
 * AC Controller API Module
 * Provides wrapper functions for AC Controller REST API calls
 * Cross-browser compatible with IE8+
 */

var ACCTL = ACCTL || {};
ACCTL.API = ACCTL.API || {};

// Ensure compat module is loaded first
if (typeof ACCTL.compat === 'undefined') {
    throw new Error('Compatibility module not loaded');
}

var XHRUtil = ACCTL.compat.XHRUtil;
var ObjectUtil = ACCTL.compat.ObjectUtil;
var DateUtil = ACCTL.compat.DateUtil;
var JSONUtil = ACCTL.compat.JSONUtil;

(function(api) {
    'use strict';

    var API_BASE = '/cgi-bin/luci/admin/network/acctl/api';
    var REQUEST_TIMEOUT = 10000;

    var metrics = {
        requests: [],
        errors: [],
        totalRequests: 0,
        successfulRequests: 0,
        failedRequests: 0,
        averageResponseTime: 0,
        maxResponseTime: 0,
        minResponseTime: Infinity
    };

    var MAX_METRICS_HISTORY = 100;

    function recordMetric(endpoint, duration, success, errorType) {
        metrics.totalRequests++;

        if (success) {
            metrics.successfulRequests++;
        } else {
            metrics.failedRequests++;
            if (errorType) {
                metrics.errors.push({
                    endpoint: endpoint,
                    type: errorType,
                    timestamp: DateUtil.now()
                });
            }
        }

        metrics.requests.push({
            endpoint: endpoint,
            duration: duration,
            success: success,
            timestamp: DateUtil.now()
        });

        if (metrics.requests.length > MAX_METRICS_HISTORY) {
            metrics.requests.shift();
        }

        if (metrics.errors.length > MAX_METRICS_HISTORY) {
            metrics.errors.shift();
        }

        var totalDuration = 0;
        for (var i = 0; i < metrics.requests.length; i++) {
            totalDuration += metrics.requests[i].duration;
        }
        metrics.averageResponseTime = totalDuration / metrics.requests.length;

        if (duration > metrics.maxResponseTime) {
            metrics.maxResponseTime = duration;
        }
        if (duration < metrics.minResponseTime) {
            metrics.minResponseTime = duration;
        }
    }

    function request(method, endpoint, params, callback) {
        if (typeof params === 'function') {
            callback = params;
            params = null;
        }

        var startTime = DateUtil.now();
        var url = API_BASE + endpoint;
        var xhr = XHRUtil.create();

        xhr.timeout = REQUEST_TIMEOUT;
        xhr.open(method.toUpperCase(), url, true);
        xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');

        xhr.onreadystatechange = function() {
            if (xhr.readyState === 4) {
                var duration = DateUtil.now() - startTime;
                if (xhr.status >= 200 && xhr.status < 300) {
                    try {
                        var data = JSONUtil.parse(xhr.responseText);
                        recordMetric(endpoint, duration, true);
                        if (typeof callback === 'function') {
                            callback(null, data);
                        }
                    } catch (e) {
                        recordMetric(endpoint, duration, false, 'JSON_PARSE_ERROR');
                        if (typeof callback === 'function') {
                            callback(new Error('Invalid JSON response'), null);
                        }
                    }
                } else {
                    recordMetric(endpoint, duration, false, 'HTTP_ERROR_' + xhr.status);
                    if (typeof callback === 'function') {
                        callback(new Error('HTTP Error: ' + xhr.status), null);
                    }
                }
            }
        };

        xhr.onerror = function() {
            var duration = DateUtil.now() - startTime;
            recordMetric(endpoint, duration, false, 'NETWORK_ERROR');
            if (typeof callback === 'function') {
                callback(new Error('Network error'), null);
            }
        };

        // Timeout handling for older browsers
        var timeoutTimer = setTimeout(function() {
            if (xhr.readyState !== 4) {
                var duration = DateUtil.now() - startTime;
                recordMetric(endpoint, duration, false, 'TIMEOUT');
                xhr.abort();
                if (typeof callback === 'function') {
                    callback(new Error('Request timeout'), null);
                }
            }
        }, REQUEST_TIMEOUT);

        xhr.onreadystatechange = (function(originalHandler) {
            return function() {
                if (xhr.readyState === 4) {
                    clearTimeout(timeoutTimer);
                }
                originalHandler();
            };
        })(xhr.onreadystatechange);

        var queryString = params ? ObjectUtil.keys(params)
            .map(function(k) {
                return encodeURIComponent(k) + '=' + encodeURIComponent(params[k]);
            })
            .join('&') : '';

        xhr.send(queryString);
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
        request('POST', '/cmd', {
            mac: mac,
            cmd: cmd
        }, callback);
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

    api.getMetrics = function() {
        return {
            totalRequests: metrics.totalRequests,
            successfulRequests: metrics.successfulRequests,
            failedRequests: metrics.failedRequests,
            averageResponseTime: Math.round(metrics.averageResponseTime),
            maxResponseTime: metrics.maxResponseTime,
            minResponseTime: metrics.minResponseTime === Infinity ? 0 : metrics.minResponseTime,
            recentRequests: metrics.requests.slice(-10),
            recentErrors: metrics.errors.slice(-10)
        };
    };

    api.getEndpointMetrics = function(endpoint) {
        var endpointRequests = [];
        for (var i = 0; i < metrics.requests.length; i++) {
            if (metrics.requests[i].endpoint === endpoint) {
                endpointRequests.push(metrics.requests[i]);
            }
        }
        return endpointRequests;
    };

    api.clearMetrics = function() {
        metrics.requests = [];
        metrics.errors = [];
        metrics.totalRequests = 0;
        metrics.successfulRequests = 0;
        metrics.failedRequests = 0;
        metrics.averageResponseTime = 0;
        metrics.maxResponseTime = 0;
        metrics.minResponseTime = Infinity;
    };

    api.API_BASE = API_BASE;
    api.REQUEST_TIMEOUT = REQUEST_TIMEOUT;

})(ACCTL.API);
