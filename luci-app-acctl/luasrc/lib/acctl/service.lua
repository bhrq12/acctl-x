module("acctl.service", package.seeall)

local sys = require "luci.sys"
local datalayer = require "acctl.datalayer"

local ALLOWED_COMMANDS = {
    ["reboot"] = true,
    ["uptime"] = true,
    ["ifconfig"] = true,
    ["iwconfig"] = true,
    ["wifi"] = true,
    ["cat /proc/uptime"] = true,
    ["cat /proc/loadavg"] = true,
    ["cat /tmp/ap_status"] = true,
}

function get_status()
    local stats = datalayer.execute_cli("stats") or {}
    return {
        running = is_running(),
        ap_online = tonumber(stats.ap_online) or 0,
        ap_total = tonumber(stats.ap_total) or 0,
        alarm_count = tonumber(stats.alarms) or 0,
        timestamp = os.time()
    }
end

function is_running()
    return luci.sys.call("pgrep -x acser > /dev/null 2>&1") == 0
end

function execute_command(mac, cmd)
    if not mac or not cmd or mac == "" or cmd == "" then
        return { code = 400, message = "Missing parameters" }
    end
    
    if not ALLOWED_COMMANDS[cmd] then
        return { code = 403, message = "Command not allowed" }
    end
    
    local safe_mac = mac:gsub("[^%x:]", ""):sub(1, 20)
    datalayer.execute_cli(string.format(
        "audit admin EXEC ap '%s' '' '%s' ''", safe_mac, cmd))
    
    return { code = 0, message = "Command queued" }
end

function get_aps(mac_filter)
    local data = datalayer.execute_cli("aps --limit 500") or {}
    local aps = {}
    
    if data.aps then
        for _, ap in ipairs(data.aps) do
            if not mac_filter or ap.mac == mac_filter then
                table.insert(aps, {
                    mac = ap.mac or "",
                    hostname = ap.hostname or "",
                    wan_ip = ap.wan_ip or "",
                    firmware = ap.firmware or "",
                    online_users = tonumber(ap.online_users) or 0,
                    online = tonumber(ap.device_down) == 0,
                    last_seen = tonumber(ap.last_seen) or 0,
                    group_id = tonumber(ap.group_id) or 0
                })
            end
        end
    end
    
    return aps
end

function get_groups()
    return datalayer.execute_cli("groups") or {}
end

function get_alarms()
    return datalayer.execute_cli("alarms") or {}
end

function get_firmwares()
    return datalayer.execute_cli("firmwares") or {}
end

function send_action(action)
    local actions = { start = true, stop = true, restart = true }
    if not actions[action] then
        return { code = 400, message = "Invalid action" }
    end
    
    luci.sys.call("/etc/init.d/acctl " .. action .. " > /dev/null 2>&1")
    return { code = 0, message = "Action executed" }
end

function restart_controller()
    luci.sys.call("/etc/init.d/acctl restart > /dev/null 2>&1")
    return { code = 0, message = "Controller restarted" }
end
