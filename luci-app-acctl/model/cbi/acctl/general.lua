--[[
AC Controller — General Settings (JSON backend)
]]

local sys  = require "luci.sys"
local uci  = require "luci.model.uci".cursor()
local datalayer = require "acctl.datalayer"

m = Map("acctl", translate("AC Controller"),
	translate("OpenWrt AC Controller — manage access points centrally"))

s = m:section(NamedSection, "acctl", "acctl", translate("General Settings"))
s.addremove = false
s.anonymous = true

enabled = s:option(Flag, "enabled", translate("Enable AC Controller"))
enabled.rmempty = false

iface = s:option(ListValue, "interface", translate("Network Interface"))
iface.widget = "select"
local interfaces = sys.net.devices() or {}
for _, iface_name in ipairs(interfaces) do
	if iface_name ~= "lo" then
		iface:value(iface_name, iface_name)
	end
end
iface.default = "br-lan"
iface.rmempty = false

port = s:option(Value, "port", translate("TCP Listen Port"),
	translate("Port for APs to connect (default: 7960)"))
port.datatype = "port"
port.default = "7960"
port.rmempty = false

pwd = s:option(Value, "password", translate("Authentication Password"),
	translate("Shared secret for AP authentication. " ..
		"Must be set before running the controller."))
pwd.password = true
pwd.rmempty = false

brditv = s:option(Value, "brditv", translate("Broadcast Interval (seconds)"),
	translate("How often to send broadcast probe packets (default: 30)"))
brditv.datatype = "uinteger"
brditv.default = "30"
brditv.rmempty = false

reschkitv = s:option(Value, "reschkitv",
	translate("IP Pool Reload Interval (seconds)"),
	translate("How often to reload IP pool from database (default: 300)"))
reschkitv.datatype = "uinteger"
reschkitv.default = "300"
reschkitv.rmempty = false

msgitv = s:option(Value, "msgitv",
	translate("Message Processing Interval (seconds)"))
msgitv.datatype = "uinteger"
msgitv.default = "3"
msgitv.rmempty = false

daemon = s:option(Flag, "daemon", translate("Run as Daemon"))
daemon.default = "1"

debug = s:option(Flag, "debug", translate("Debug Mode"))
debug.rmempty = true

s2 = m:section(NamedSection, "acctl", "acctl",
	translate("AC Information (Read Only)"))
s2.addremove = false
s2.anonymous = true

uuid_info = s2:option(DummyValue, "_uuid", translate("AC UUID"))
uuid_info.template = "acctl/ap_info"

s3 = m:section(Table, {}, translate("Access Point Statistics"))

local stats = datalayer.execute_cli("stats") or {}
local ap_total    = tonumber(stats.ap_total) or 0
local ap_online   = tonumber(stats.ap_online) or 0
local alarm_count = tonumber(stats.alarms) or 0

s3:option(DummyValue, "_total",   translate("Total APs"),   tostring(ap_total))
s3:option(DummyValue, "_online",  translate("Online APs"),   tostring(ap_online))
s3:option(DummyValue, "_offline", translate("Offline APs"), tostring(ap_total - ap_online))
s3:option(DummyValue, "_alarms", translate("Active Alarms"), tostring(alarm_count))

if sys.call("pgrep acser > /dev/null 2>&1") == 0 then
	m.on_commit = function(map)
		luci.sys.exec("/etc/init.d/acctl restart")
	end
else
	m.on_init = function(map)
		if sys.call("test -e /etc/config/acctl") == 0 then
		end
	end
end

return m
