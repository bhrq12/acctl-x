--[[
AC Controller — AP List Management (JSON backend)
Dual-band and multi-SSID support
Single-band AP compatibility
]]

local sys       = require "luci.sys"
local luci_http = require "luci.http"
local datalayer = require "acctl.datalayer"

m = Map("acctl", translate("AP List"),
	translate("Manage connected Access Points"))

local function get_aps()
	local aps = {}
	local data = datalayer.execute_cli("aps --limit 500") or {}
	if not data or not data.aps then
		return aps
	end
	for _, t in ipairs(data.aps) do
		local last_seen = tonumber(t.last_seen) or 0
		local last_seen_str = "Never"
		if last_seen > 0 then
			last_seen_str = os.date("%Y-%m-%d %H:%M:%S", last_seen)
		end

		local ssid_count = tonumber(t.ssid_count) or 0
		local wifi_ssid = t.wifi_ssid or ""
		local wifi_channel = t.wifi_channel or ""
		local wifi_encryption = t.wifi_encryption or ""
		local band_type = "2.4GHz"
		if wifi_channel and wifi_channel ~= "" and tonumber(wifi_channel) and tonumber(wifi_channel) > 14 then
			band_type = "5GHz"
		elseif ssid_count > 1 then
			band_type = "dual"
		end

		table.insert(aps, {
			mac            = t.mac or "",
			hostname       = t.hostname or "",
			wan_ip         = t.wan_ip or "",
			wifi_ssid      = wifi_ssid,
			wifi_channel   = wifi_channel,
			wifi_encryption = wifi_encryption,
			ssid_count     = ssid_count,
			band_type      = band_type,
			firmware       = t.firmware or "",
			online_users   = tonumber(t.online_users) or 0,
			device_down    = tonumber(t.device_down) or 1,
			last_seen      = last_seen,
			last_seen_str  = last_seen_str,
			group_id       = tonumber(t.group_id) or 0,
		})
	end
	return aps
end

local all_aps = get_aps()
s = m:section(Table, all_aps,
	translatef("Access Points (%d total)", #all_aps))

s:option(DummyValue, "mac", translate("MAC Address"))
s:option(DummyValue, "hostname", translate("Hostname"))

status_col = s:option(DummyValue, "device_down", translate("Status"))
status_col.template = "acctl/ap_status_badge"

s:option(DummyValue, "wan_ip", translate("WAN IP"))

wifi_col = s:option(DummyValue, "wifi_ssid", translate("WiFi SSIDs"))
wifi_col.template = "acctl/ap_wifi"

band_col = s:option(DummyValue, "band_type", translate("Band"))
band_col.template = "acctl/ap_band"

s:option(DummyValue, "firmware", translate("Firmware"))

users_col = s:option(DummyValue, "online_users", translate("Users"))
users_col.template = "acctl/user_badge"

s:option(DummyValue, "last_seen_str", translate("Last Seen"))
s:option(DummyValue, "group_id", translate("Group"))

function s.render_footer(self)
	luci_http.write('<div class="cbi-page-actions">')
	luci_http.write('<input class="cbi-button cbi-button-action important" ' ..
		'type="submit" name="refresh" value="' .. translate("Refresh") .. '" />')
	luci_http.write('</div>')
end

s2 = m:section(NamedSection, "acctl", "acctl",
	translate("Bulk Operations"))
s2.addremove = false

sel = s2:option(Value, "_selected", translate("Selected APs (comma-separated MACs)"))
sel.placeholder = "aa:bb:cc:dd:ee:ff, 11:22:33:44:55:66"
sel.datatype = "string"

action = s2:option(ListValue, "_action", translate("Action"))
action:value("none",     translate("Select action..."))
action:value("reboot",  translate("Reboot APs"))
action:value("upgrade",  translate("Upgrade Firmware"))
action:value("config",  translate("Push Configuration"))
action:value("offline",  translate("Mark Offline"))

apply_btn = s2:option(Button, "_apply", translate("Apply"))
apply_btn.inputstyle = "apply"

return m
