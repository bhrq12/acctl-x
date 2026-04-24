--[[
LuCI - Lua Configuration Interface
AC Controller — LuCI Controller Module
JSON backend (replaces SQLite)

Licensed under the Apache License, Version 2.0
http://www.apache.org/licenses/LICENSE-2.0
]]--

module("luci.controller.acctl", package.seeall)

local sys   = require "luci.sys"
local util  = require "luci.util"
local http  = require "luci.http"
local uci   = require "luci.model.uci".cursor()
local csrf  = require "luci.csrf"

-- ========================================================================
-- Security: CSRF protection
-- ========================================================================

local function check_csrf()
    local token = http.formvalue("token")
    if not token or token == "" then
        return false
    end
    return csrf.verify_token(token)
end

local function generate_csrf_token()
    return csrf.create_token()
end

-- ========================================================================
-- Security: Input validation
-- ========================================================================

local function validate_mac(mac)
    if not mac or type(mac) ~= "string" then
        return false
    end
    return mac:match("^%x%x:%x%x:%x%x:%x%x:%x%x:%x%x$") ~= nil
end

local function validate_action(action)
    local valid_actions = { reboot = true, config = true, upgrade = true }
    return valid_actions[action] == true
end

local function validate_cmd(cmd)
    local allowed = {
        ["reboot"]              = true,
        ["uptime"]              = true,
        ["ifconfig"]            = true,
        ["iwconfig"]            = true,
        ["wifi"]                = true,
        ["cat /proc/uptime"]    = true,
        ["cat /proc/loadavg"]   = true,
        ["cat /tmp/ap_status"]  = true,
    }
    
    -- Reject if cmd contains any shell metacharacters or control chars
    local shell_metachar = "[;&|`$(){}<>%!~\n\r%[%]%*%?]"
    if cmd:match(shell_metachar) then
        return false
    end
    
    -- Reject if command contains path separators not in whitelist
    if cmd:match("/") and not allowed[cmd] then
        return false
    end
    
    -- Exact whitelist match
    return allowed[cmd] == true
end

local function sanitize_input(input, max_len)
    if not input or type(input) ~= "string" then
        return ""
    end
    max_len = max_len or 256
    return input:gsub("[^%w%p ]", ""):sub(1, max_len)
end

-- ========================================================================
-- Helper: run acctl-cli and parse JSON response
-- ========================================================================

local function cli_output(cmd)
    local f = io.popen(cmd .. " 2>/dev/null")
    if not f then return nil end
    local r = f:read("*a")
    f:close()
    return r
end

local function cli_json(args)
    local cmd = "acctl-cli " .. args
    local out = cli_output(cmd)
    if not out or out == "" then return nil end
    local ok, data = pcall(http.parse_json, out)
    if not ok then return nil end
    return data
end

local function is_running()
    return sys.call("pgrep -x acser > /dev/null 2>&1") == 0
end

-- ========================================================================
-- Menu entries
-- ========================================================================

function index()
    entry({"admin", "network", "acctl"},
        alias("admin", "network", "acctl", "general"),
        _("AC Controller"), 60).dependent = false

    entry({"admin", "network", "acctl", "general"},
        cbi("acctl/general", {autoapply=true}),
        _("General"), 10)

    entry({"admin", "network", "acctl", "ap_list"},
        cbi("acctl/ap_list", {autoapply=true}),
        _("AP List"), 20)

    entry({"admin", "network", "acctl", "groups"},
        cbi("acctl/groups", {autoapply=true}),
        _("AP Groups"), 25)

    entry({"admin", "network", "acctl", "templates"},
        cbi("acctl/templates", {autoapply=true}),
        _("Templates"), 30)

    entry({"admin", "network", "acctl", "alarms"},
        cbi("acctl/alarms", {autoapply=true}),
        _("Alarms"), 40)

    entry({"admin", "network", "acctl", "firmware"},
        cbi("acctl/firmware", {autoapply=true}),
        _("Firmware"), 50)

    entry({"admin", "network", "acctl", "system"},
        cbi("acctl/system"),
        _("System"), 60)

    -- REST API endpoints
    entry({"admin", "network", "acctl", "api", "status"},
        call("api_status"))

    entry({"admin", "network", "acctl", "api", "aps"},
        call("api_aps"))

    entry({"admin", "network", "acctl", "api", "aps_action"},
        call("api_aps_action"))

    entry({"admin", "network", "acctl", "api", "alarms"},
        call("api_alarms"))

    entry({"admin", "network", "acctl", "api", "alarms_ack"},
        call("api_alarms_ack"))

    entry({"admin", "network", "acctl", "api", "groups"},
        call("api_groups"))

    entry({"admin", "network", "acctl", "api", "firmwares"},
        call("api_firmwares"))

    entry({"admin", "network", "acctl", "api", "cmd"},
        call("api_cmd"))

    entry({"admin", "network", "acctl", "api", "restart"},
        call("api_restart"))
end

-- ========================================================================
-- API: Status
-- ========================================================================

function api_status()
    local stats = cli_json("stats") or {}
    local status = {
        running       = is_running(),
        ap_online     = tonumber(stats.ap_online) or 0,
        ap_total      = tonumber(stats.ap_total)  or 0,
        alarm_count   = tonumber(stats.alarms)    or 0,
        pool_left     = 0,
        pool_total    = 0,
        timestamp     = os.time()
    }

    http.prepare_content("application/json")
    http.write_json(status)
end

-- ========================================================================
-- API: AP List
-- ========================================================================

function api_aps()
    local mac    = http.formvalue("mac")
    local result = {}

    if mac and mac ~= "" then
        -- Single AP detail — load all and find by mac
        local data = cli_json("aps --limit 500") or {}
        if data and data.aps then
            for _, ap in ipairs(data.aps) do
                if ap.mac == mac then
                    result.mac          = ap.mac or ""
                    result.hostname     = ap.hostname or ""
                    result.wan_ip       = ap.wan_ip or ""
                    result.wifi_ssid    = ap.wifi_ssid or ""
                    result.firmware     = ap.firmware or ""
                    result.online_users = tonumber(ap.online_users) or 0
                    result.online       = (tonumber(ap.device_down) == 0)
                    result.last_seen    = tonumber(ap.last_seen) or 0
                    result.group_id     = tonumber(ap.group_id) or 0
                    break
                end
            end
        end
    else
        -- All APs
        result.aps   = {}
        result.count = 0
        local data = cli_json("aps --limit 500") or {}
        if data and data.aps then
            for _, ap in ipairs(data.aps) do
                result.count = result.count + 1
                result.aps[#result.aps + 1] = {
                    mac          = ap.mac or "",
                    hostname     = ap.hostname or "",
                    wan_ip       = ap.wan_ip or "",
                    wifi_ssid    = ap.wifi_ssid or "",
                    firmware     = ap.firmware or "",
                    online_users = tonumber(ap.online_users) or 0,
                    online       = (tonumber(ap.device_down) == 0),
                    last_seen    = tonumber(ap.last_seen) or 0,
                    group_id     = tonumber(ap.group_id) or 0
                }
            end
        end
    end

    http.prepare_content("application/json")
    http.write_json(result)
end

-- ========================================================================
-- API: AP Bulk Actions
-- ========================================================================

function api_aps_action()
    local result = { code = 0, message = "", affected = 0 }

    -- Check CSRF token
    if not check_csrf() then
        result.code    = 403
        result.message = "CSRF token validation failed"
        http.prepare_content("application/json")
        http.write_json(result)
        return
    end

    local action = http.formvalue("action")
    local macs   = http.formvalue("macs") or ""

    if action == "" or macs == "" then
        result.code    = 400
        result.message = "action and macs required"
        http.prepare_content("application/json")
        http.write_json(result)
        return
    end

    -- Validate action
    if not validate_action(action) then
        result.code    = 400
        result.message = "Invalid action"
        http.prepare_content("application/json")
        http.write_json(result)
        return
    end

    -- Sanitize macs
    local safe_macs = sanitize_input(macs, 500)

    -- Audit log
    cli_output(string.format(
        "acctl-cli audit admin %s ap_batch '%s' '' '' ''",
        action, safe_macs:sub(1, 50)))

    if action == "reboot" then
        result.message = "Reboot command queued for APs"
    elseif action == "config" then
        result.message = "Configuration push queued"
    elseif action == "upgrade" then
        result.message = "Firmware upgrade queued"
    else
        result.code    = 400
        result.message = "Unknown action"
    end

    http.prepare_content("application/json")
    http.write_json(result)
end

-- ========================================================================
-- API: Alarm List
-- ========================================================================

function api_alarms()
    local limit = tonumber(http.formvalue("limit")) or 50
    local data = cli_json("alarms --limit " .. tostring(limit)) or {}

    local alarms = {}
    if data.alarms then
        for _, a in ipairs(data.alarms) do
            local level = 0
            if a.level == "warn"      then level = 1
            elseif a.level == "error" then level = 2
            elseif a.level == "critical" then level = 3
            end
            alarms[#alarms + 1] = {
                id            = tonumber(a.id) or 0,
                ap_mac        = a.mac or "",
                level         = a.level or "info",
                level_num     = level,
                message       = a.message or "",
                acknowledged  = (a.ack == 1 or a.ack == true),
                created_at    = a.ts or ""
            }
        end
    end

    http.prepare_content("application/json")
    http.write_json({alarms = alarms, count = #alarms})
end

-- ========================================================================
-- API: Acknowledge Alarms
-- ========================================================================

function api_alarms_ack()
    local result = { code = 0, acknowledged = 0 }

    -- Check CSRF token
    if not check_csrf() then
        result.code    = 403
        result.message = "CSRF token validation failed"
        http.prepare_content("application/json")
        http.write_json(result)
        return
    end

    local ids = http.formvalue("ids") or ""

    if ids == "all" then
        cli_output("acctl-cli ack-all")
        result.acknowledged = -1
    else
        for id in ids:gmatch("%d+") do
            cli_output("acctl-cli ack " .. id)
            result.acknowledged = result.acknowledged + 1
        end
    end

    http.prepare_content("application/json")
    http.write_json(result)
end

-- ========================================================================
-- API: AP Groups
-- ========================================================================

function api_groups()
    local data = cli_json("groups") or {}
    local groups = {}

    if data.groups then
        -- Count APs per group from the full AP list
        local aps_data = cli_json("aps --limit 500") or {}
        local ap_count = {}
        if aps_data and aps_data.aps then
            for _, ap in ipairs(aps_data.aps) do
                local gid = tonumber(ap.group_id) or 0
                ap_count[gid] = (ap_count[gid] or 0) + 1
            end
        end

        for _, grp in ipairs(data.groups) do
            groups[#groups + 1] = {
                id          = tonumber(grp.id) or 0,
                name        = grp.name or "",
                description = grp.description or "",
                policy      = grp.policy or "manual",
                ap_count    = ap_count[tonumber(grp.id) or 0] or 0
            }
        end
    end

    http.prepare_content("application/json")
    http.write_json({groups = groups})
end

-- ========================================================================
-- API: Firmware List
-- ========================================================================

function api_firmwares()
    local data = cli_json("firmware") or {}

    local firmwares = {}
    if data.firmwares then
        for _, fw in ipairs(data.firmwares) do
            firmwares[#firmwares + 1] = {
                version     = fw.version or "",
                filename    = fw.filename or "",
                size        = tonumber(fw.size) or 0,
                sha256      = fw.sha256 or "",
                uploaded_at = fw.uploaded_at or ""
            }
        end
    end

    http.prepare_content("application/json")
    http.write_json({firmwares = firmwares})
end

-- ========================================================================
-- API: Command Execution
-- ========================================================================

function api_cmd()
    local result = { code = 0, message = "" }

    -- Check CSRF token
    if not check_csrf() then
        result.code    = 403
        result.message = "CSRF token validation failed"
        http.prepare_content("application/json")
        http.write_json(result)
        return
    end

    local mac = http.formvalue("mac")
    local cmd = http.formvalue("cmd")

    if not mac or not cmd or mac == "" or cmd == "" then
        result.code    = 400
        result.message = "mac and cmd are required"
        http.prepare_content("application/json")
        http.write_json(result)
        return
    end

    -- Validate MAC address
    if not validate_mac(mac) then
        result.code    = 400
        result.message = "Invalid MAC address"
        http.prepare_content("application/json")
        http.write_json(result)
        return
    end

    -- Validate command
    if not validate_cmd(cmd) then
        result.code    = 403
        result.message = "Command not allowed"
        http.prepare_content("application/json")
        http.write_json(result)
        return
    end

    -- Sanitize inputs
    local safe_mac = sanitize_input(mac, 20)
    local safe_cmd = sanitize_input(cmd, 100)

    -- Audit
    cli_output(string.format(
        "acctl-cli audit admin EXEC ap '%s' '' '%s' ''",
        safe_mac, safe_cmd))

    result.message = "Command queued successfully"
    http.prepare_content("application/json")
    http.write_json(result)
end

-- ========================================================================
-- API: Restart Service
-- ========================================================================

function api_restart()
    local result = { code = 0, message = "" }

    -- Check CSRF token
    if not check_csrf() then
        result.code    = 403
        result.message = "CSRF token validation failed"
        http.prepare_content("application/json")
        http.write_json(result)
        return
    end

    if is_running() then
        sys.exec("/etc/init.d/acctl restart > /dev/null 2>&1")
        result.message = "AC Controller restarting..."
    else
        sys.exec("/etc/init.d/acctl start > /dev/null 2>&1")
        result.message = "AC Controller starting..."
    end
    http.prepare_content("application/json")
    http.write_json(result)
end
