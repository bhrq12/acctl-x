--[[
AC Controller — AP Groups (JSON backend)
]]

local sys       = require "luci.sys"
local http      = require "luci.http"
local datalayer = require "acctl.datalayer"

m = Map("acctl", translate("AP Groups"),
	translate("Organize Access Points into groups for batch management"))

local function get_groups()
	local groups = {}
	local data = datalayer.execute_cli("groups") or {}
	if not data or not data.groups then return groups end

	-- Count APs per group from AP list
	local aps_data = datalayer.execute_cli("aps --limit 500") or {}
	local ap_count = {}
	if aps_data and aps_data.aps then
		for _, ap in ipairs(aps_data.aps) do
			local gid = tonumber(ap.group_id) or 0
			ap_count[gid] = (ap_count[gid] or 0) + 1
		end
	end

	for _, grp in ipairs(data.groups) do
		table.insert(groups, {
			id          = tonumber(grp.id) or 0,
			name        = grp.name or "",
			description = grp.description or "",
			policy      = grp.policy or "manual",
			ap_count    = ap_count[tonumber(grp.id) or 0] or 0
		})
	end
	return groups
end

s = m:section(TypedSection, "profile", translate("Groups"),
	translate("Create and manage AP groups"))
s.anonymous = true
s.addremove = true

name = s:option(Value, "name", translate("Group Name"))
name.rmempty = false

desc = s:option(Value, "description", translate("Description"))
desc.placeholder = translate("Optional description")

policy = s:option(ListValue, "update_policy", translate("Update Policy"))
policy:value("manual", translate("Manual (no auto-upgrade)"))
policy:value("auto", translate("Auto (apply template on AP connect)"))
policy:value("rolling", translate("Rolling (upgrade one at a time)"))
policy.default = "manual"

-- Group list display
local grp_list = get_groups()
s2 = m:section(Table, grp_list,
	translatef("Groups (%d)", #grp_list))

s2:option(DummyValue, "id", translate("ID"))
s2:option(DummyValue, "name", translate("Name"))
s2:option(DummyValue, "description", translate("Description"))
s2:option(DummyValue, "policy", translate("Policy"))
s2:option(DummyValue, "ap_count", translate("AP Count"))

return m
