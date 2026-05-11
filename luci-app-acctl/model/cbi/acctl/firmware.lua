--[[
AC Controller — Firmware Repository (JSON backend)
]]

local sys       = require "luci.sys"
local util      = require "luci.util"
local http      = require "luci.http"
local datalayer = require "acctl.datalayer"

m = Map("acctl", translate("Firmware"),
	translate("Manage AP firmware images"))

-- Firmware table
local firmwares = {}
local data = datalayer.execute_cli("firmware") or {}
if data and data.firmwares then
	for _, fw in ipairs(data.firmwares) do
		table.insert(firmwares, {
			version     = fw.version or "",
			filename    = fw.filename or "",
			size        = tonumber(fw.size) or 0,
			size_str    = string.format("%.1f MB", (tonumber(fw.size) or 0) / 1024 / 1024),
			sha256      = fw.sha256 or "",
			uploaded_at = fw.uploaded_at or ""
		})
	end
end

s = m:section(Table, firmwares,
	translatef("Available Firmware (%d)", #firmwares))

s:option(DummyValue, "version",    translate("Version"))
s:option(DummyValue, "filename",   translate("Filename"))
s:option(DummyValue, "size_str",   translate("Size"))
s:option(DummyValue, "uploaded_at", translate("Uploaded"))

return m
