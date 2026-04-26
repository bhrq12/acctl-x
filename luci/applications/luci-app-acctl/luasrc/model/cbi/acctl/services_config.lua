--[[
AC Controller - Services Configuration Page
]]--

module("luci.controller.acctl", package.seeall)

local fs   = require "nixio.fs"
local sys  = require "luci.sys"
local uci  = require "luci.model.uci".cursor()
local util = require "luci.util"

m = Map("acctl", translate("AC Controller Configuration"),
    translate("Configure AC Controller service settings"))

s = m:section(TypedSection, "acctl", translate("Service Settings"))
s.anonymous = true
s.addremove = false

-- Mode selection
mode = s:option(ListValue, "mode", translate("Mode"),
    translate("Select AC or AP mode"))
mode:value("ac", translate("AC Mode"))
mode:value("ap", translate("AP Mode"))
mode.default = "ac"

-- Enable/disable service
enabled = s:option(Flag, "enabled", translate("Enabled"),
    translate("Enable AC Controller service"))
enabled.default = true

-- Network interface
interface = s:option(Value, "interface", translate("Interface"),
    translate("Network interface to use"))
for _, iface in ipairs(sys.net.devices()) do
    if iface ~= "lo" then
        interface:value(iface)
    end
end
interface.default = "br-lan"

-- Port
port = s:option(Value, "port", translate("Port"),
    translate("Service port"))
port.datatype = "port"
port.default = "7960"

-- Password
password = s:option(Value, "password", translate("Password"),
    translate("Administrator password"))
password.password = true
password.default = "acctl@2024"

-- Broadcast interval
brditv = s:option(Value, "brditv", translate("Broadcast Interval"),
    translate("Interval in seconds for AC broadcast probes"))
brditv.datatype = "uinteger"
brditv.default = "30"

-- IP pool refresh interval
reschkitv = s:option(Value, "reschkitv", translate("IP Pool Refresh Interval"),
    translate("Interval in seconds for IP pool refresh"))
reschkitv.datatype = "uinteger"
reschkitv.default = "300"

-- Message processing interval
msgitv = s:option(Value, "msgitv", translate("Message Processing Interval"),
    translate("Interval in seconds for message processing"))
msgitv.datatype = "uinteger"
msgitv.default = "3"

-- Daemon mode
daemon = s:option(Flag, "daemon", translate("Daemon Mode"),
    translate("Run as daemon"))
daemon.default = true

-- Debug mode
debug = s:option(Flag, "debug", translate("Debug Mode"),
    translate("Enable debug logging"))
debug.default = false

function m.on_commit(map) 
    -- Restart service when configuration changes
    sys.exec("/etc/init.d/acctl restart > /dev/null 2>&1")
end

return m