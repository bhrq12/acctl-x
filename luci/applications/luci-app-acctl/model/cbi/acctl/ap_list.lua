--[[
AC Controller — AP List Management (JSON backend)
Dual-band and multi-SSID support
Single-band AP compatibility
]]

local sys       = require "luci.sys"
local util      = require "luci.util"
local luci_http = require "luci.http"

m = Map("acctl", translate("AP List"),
	translate("Manage connected Access Points"))

local function get_aps()
	local aps = {}
	local output = sys.exec("acctl-cli aps --limit 500 2>/dev/null")
	local ok, data = pcall(luci_http.parse_json, output)
	if not ok or not data or not data.aps then
		return aps
	end
	for _, t in ipairs(data.aps) do
		local last_seen = tonumber(t.last_seen) or 0
		local last_seen_str = "Never"
		if last_seen > 0 then
			last_seen_str = os.date("%Y-%m-%d %H:%M:%S", last_seen)
		end

		local ssid_count = tonumber(t.ssid_count) or 0
		local ssids_data = t.ssids or {}
		local ssid_2g = ""
		local ssid_5g = ""
		local clients_2g = 0
		local clients_5g = 0
		local band_2g = ""
		local band_5g = ""
		local has_2g = false
		local has_5g = false

		for _, s in ipairs(ssids_data) do
			local band = s.band or ""
			if band:match("2.4") then
				ssid_2g = s.ssid or ""
				clients_2g = tonumber(s.clients) or 0
				band_2g = band
				has_2g = true
			elseif band:match("5") then
				ssid_5g = s.ssid or ""
				clients_5g = tonumber(s.clients) or 0
				band_5g = band
				has_5g = true
			elseif band == "" or band:match("Unknown") or not s.channel or s.channel == 0 then
				if not has_2g and not has_5g then
					ssid_2g = s.ssid or ""
					clients_2g = tonumber(s.clients) or 0
					has_2g = true
				end
			end
		end

		if ssid_count == 0 or (not has_2g and not has_5g) then
			local wifi_ssid = t.wifi_ssid or ""
			if wifi_ssid ~= "" and wifi_ssid ~= "OpenWrt-AP" then
				ssid_2g = wifi_ssid
				has_2g = true
			end
		end

		local is_single_band = (has_2g and not has_5g) or (has_5g and not has_2g)
		local band_type = "dual"
		if is_single_band then
			band_type = has_5g and "5GHz" or "2.4GHz"
		end

		table.insert(aps, {
			mac            = t.mac or "",
			hostname       = t.hostname or "",
			wan_ip         = t.wan_ip or "",
			wifi_ssid      = t.wifi_ssid or "",
			ssid_2g        = ssid_2g,
			ssid_5g        = ssid_5g,
			clients_2g     = clients_2g,
			clients_5g     = clients_5g,
			band_2g        = band_2g,
			band_5g        = band_5g,
			ssid_count     = ssid_count,
			band_type      = band_type,
			is_single_band = is_single_band,
			firmware       = t.firmware or "",
			online_users   = tonumber(t.online_users) or 0,
			device_down    = tonumber(t.device_down) or 1,
			last_seen      = last_seen,
			last_seen_str  = last_seen_str,
			group_id       = tonumber(t.group_id) or 0,
			ssids_data     = ssids_data
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
status_col.template = "acctl/ap_status"

s:option(DummyValue, "wan_ip", translate("WAN IP"))

wifi_col = s:option(DummyValue, "wifi_ssid", translate("WiFi SSIDs"))
wifi_col.template = "acctl/ap_wifi"

band_col = s:option(DummyValue, "band_type", translate("Band"))
band_col.template = "acctl/ap_band"

s:option(DummyValue, "firmware", translate("Firmware"))

users_col = s:option(DummyValue, "online_users", translate("Users"))
users_col.template = "acctl/ap_users"

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