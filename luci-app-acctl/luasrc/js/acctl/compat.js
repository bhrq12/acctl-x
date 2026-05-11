/**
 * AC Controller Compatibility Module
 * Provides cross-browser compatibility utilities and polyfills
 * Supports: IE8+, Chrome 80+, Firefox 75+, Safari 13+, Edge 80+
 */

var ACCTL = ACCTL || {};
ACCTL.compat = ACCTL.compat || {};

(function(compat) {
    'use strict';

    /**
     * Event utility for cross-browser event handling
     */
    compat.EventUtil = {
        addListener: function(element, type, handler) {
            if (element.addEventListener) {
                element.addEventListener(type, handler, false);
            } else if (element.attachEvent) {
                element.attachEvent('on' + type, handler);
            } else {
                element['on' + type] = handler;
            }
        },
        
        removeListener: function(element, type, handler) {
            if (element.removeEventListener) {
                element.removeEventListener(type, handler, false);
            } else if (element.detachEvent) {
                element.detachEvent('on' + type, handler);
            } else {
                element['on' + type] = null;
            }
        },
        
        getEvent: function(event) {
            return event || window.event;
        },
        
        getTarget: function(event) {
            var e = compat.EventUtil.getEvent(event);
            return e.target || e.srcElement;
        }
    };

    /**
     * DOM utility for cross-browser DOM manipulation
     */
    compat.DOMUtil = {
        getById: function(id) {
            return document.getElementById(id);
        },
        
        querySelector: function(selector, context) {
            var ctx = context || document;
            if (ctx.querySelector) {
                return ctx.querySelector(selector);
            }
            return null;
        },
        
        querySelectorAll: function(selector, context) {
            var ctx = context || document;
            if (ctx.querySelectorAll) {
                return ctx.querySelectorAll(selector);
            }
            return [];
        },
        
        getDataAttr: function(element, name) {
            if (element.dataset) {
                return element.dataset[name];
            }
            return element.getAttribute('data-' + name);
        },
        
        setDataAttr: function(element, name, value) {
            if (element.dataset) {
                element.dataset[name] = value;
            } else {
                element.setAttribute('data-' + name, value);
            }
        },
        
        addClass: function(element, className) {
            if (element.classList) {
                element.classList.add(className);
            } else {
                var classes = element.className.split(' ');
                if (classes.indexOf(className) === -1) {
                    classes.push(className);
                    element.className = classes.join(' ');
                }
            }
        },
        
        removeClass: function(element, className) {
            if (element.classList) {
                element.classList.remove(className);
            } else {
                var classes = element.className.split(' ');
                var index = classes.indexOf(className);
                if (index > -1) {
                    classes.splice(index, 1);
                    element.className = classes.join(' ');
                }
            }
        },
        
        hasClass: function(element, className) {
            if (element.classList) {
                return element.classList.contains(className);
            }
            return element.className.indexOf(className) > -1;
        },
        
        createElement: function(tagName, options) {
            var el = document.createElement(tagName);
            if (options) {
                if (options.className) {
                    el.className = options.className;
                }
                if (options.textContent) {
                    el.textContent = options.textContent;
                }
                if (options.innerHTML) {
                    el.innerHTML = options.innerHTML;
                }
            }
            return el;
        }
    };

    /**
     * XHR utility for cross-browser AJAX requests
     */
    compat.XHRUtil = {
        create: function() {
            if (typeof XMLHttpRequest !== 'undefined') {
                return new XMLHttpRequest();
            } else if (typeof ActiveXObject !== 'undefined') {
                var versions = [
                    'MSXML2.XMLHttp.6.0',
                    'MSXML2.XMLHttp.3.0',
                    'MSXML2.XMLHttp'
                ];
                for (var i = 0; i < versions.length; i++) {
                    try {
                        return new ActiveXObject(versions[i]);
                    } catch (e) {
                        // Ignore
                    }
                }
            }
            throw new Error('XMLHttpRequest not supported');
        }
    };

    /**
     * Array polyfills for ES5 compatibility
     */
    compat.ArrayUtil = {
        // Polyfill for Array.prototype.filter
        filter: function(array, callback, thisArg) {
            if (Array.prototype.filter) {
                return array.filter(callback, thisArg);
            }
            
            var result = [];
            for (var i = 0; i < array.length; i++) {
                if (callback.call(thisArg, array[i], i, array)) {
                    result.push(array[i]);
                }
            }
            return result;
        },
        
        // Polyfill for Array.prototype.forEach
        forEach: function(array, callback, thisArg) {
            if (Array.prototype.forEach) {
                return array.forEach(callback, thisArg);
            }
            
            for (var i = 0; i < array.length; i++) {
                callback.call(thisArg, array[i], i, array);
            }
        },
        
        // Polyfill for Array.prototype.map
        map: function(array, callback, thisArg) {
            if (Array.prototype.map) {
                return array.map(callback, thisArg);
            }
            
            var result = [];
            for (var i = 0; i < array.length; i++) {
                result.push(callback.call(thisArg, array[i], i, array));
            }
            return result;
        }
    };

    /**
     * Object utility for cross-browser compatibility
     */
    compat.ObjectUtil = {
        keys: function(obj) {
            if (Object.keys) {
                return Object.keys(obj);
            }
            
            var result = [];
            for (var prop in obj) {
                if (obj.hasOwnProperty(prop)) {
                    result.push(prop);
                }
            }
            return result;
        },
        
        assign: function(target) {
            if (Object.assign) {
                return Object.assign.apply(Object, arguments);
            }
            
            for (var i = 1; i < arguments.length; i++) {
                var source = arguments[i];
                for (var prop in source) {
                    if (source.hasOwnProperty(prop)) {
                        target[prop] = source[prop];
                    }
                }
            }
            return target;
        }
    };

    /**
     * Date utility for cross-browser compatibility
     */
    compat.DateUtil = {
        now: function() {
            if (Date.now) {
                return Date.now();
            }
            return new Date().getTime();
        },
        
        toLocaleString: function(date) {
            if (date.toLocaleString) {
                return date.toLocaleString();
            }
            return date.toString();
        }
    };

    /**
     * JSON utility for cross-browser compatibility
     */
    compat.JSONUtil = {
        parse: function(str) {
            if (window.JSON && JSON.parse) {
                return JSON.parse(str);
            }
            // Fallback for very old browsers
            return eval('(' + str + ')');
        },
        
        stringify: function(obj) {
            if (window.JSON && JSON.stringify) {
                return JSON.stringify(obj);
            }
            // Simple fallback
            return '{}';
        }
    };

    /**
     * String utility for cross-browser compatibility
     */
    compat.StringUtil = {
        trim: function(str) {
            if (String.prototype.trim) {
                return str.trim();
            }
            return str.replace(/^\s+|\s+$/g, '');
        },
        
        startsWith: function(str, searchString) {
            if (String.prototype.startsWith) {
                return str.startsWith(searchString);
            }
            return str.indexOf(searchString) === 0;
        },
        
        endsWith: function(str, searchString) {
            if (String.prototype.endsWith) {
                return str.endsWith(searchString);
            }
            var pos = str.length - searchString.length;
            return pos >= 0 && str.indexOf(searchString, pos) === pos;
        }
    };

    /**
     * Window utility for cross-browser compatibility
     */
    compat.WindowUtil = {
        addLoadEvent: function(func) {
            var oldOnload = window.onload;
            if (typeof window.onload !== 'function') {
                window.onload = func;
            } else {
                window.onload = function() {
                    oldOnload();
                    func();
                };
            }
        }
    };

    /**
     * Initialize polyfills
     */
    compat.initPolyfills = function() {
        // Array.prototype.filter
        if (!Array.prototype.filter) {
            Array.prototype.filter = function(callback, thisArg) {
                return compat.ArrayUtil.filter(this, callback, thisArg);
            };
        }

        // Array.prototype.forEach
        if (!Array.prototype.forEach) {
            Array.prototype.forEach = function(callback, thisArg) {
                compat.ArrayUtil.forEach(this, callback, thisArg);
            };
        }

        // Array.prototype.map
        if (!Array.prototype.map) {
            Array.prototype.map = function(callback, thisArg) {
                return compat.ArrayUtil.map(this, callback, thisArg);
            };
        }

        // Object.keys
        if (!Object.keys) {
            Object.keys = function(obj) {
                return compat.ObjectUtil.keys(obj);
            };
        }

        // Date.now
        if (!Date.now) {
            Date.now = function() {
                return compat.DateUtil.now();
            };
        }

        // String.prototype.trim
        if (!String.prototype.trim) {
            String.prototype.trim = function() {
                return compat.StringUtil.trim(this);
            };
        }
    };

})(ACCTL.compat);

// Initialize polyfills on load
ACCTL.compat.initPolyfills();
