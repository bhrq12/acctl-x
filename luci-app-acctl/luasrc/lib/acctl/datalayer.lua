module("acctl.datalayer", package.seeall)

local sys = require "luci.sys"
local http = require "luci.http"

function html_escape(s)
    if not s then return "" end
    return tostring(s)
        :gsub("&", "&amp;")
        :gsub("<", "&lt;")
        :gsub(">", "&gt;")
        :gsub('"', "&quot;")
        :gsub("'", "&#039;")
end

function html_escape_recursive(data)
    if type(data) == "table" then
        local result = {}
        for k, v in pairs(data) do
            result[k] = html_escape_recursive(v)
        end
        return result
    elseif type(data) == "string" then
        return html_escape(data)
    else
        return data
    end
end

local cache = {}
local CACHE_TIMEOUT = 30

function execute_cli(cmd, escape)
    local cached = get_cached(cmd)
    if cached then return cached end
    
    local output = sys.exec("acctl-cli " .. cmd .. " 2>/dev/null")
    if not output or output == "" then
        set_cached(cmd, nil)
        return nil
    end
    
    local ok, data = pcall(http.parse_json, output)
    if not ok then
        set_cached(cmd, nil)
        return nil
    end
    
    if escape then
        data = html_escape_recursive(data)
    end
    
    set_cached(cmd, data)
    return data
end

function get_cached(key)
    local now = os.time()
    if cache[key] and cache[key].timestamp + CACHE_TIMEOUT > now then
        return cache[key].data
    end
    return nil
end

function set_cached(key, data)
    cache[key] = {
        data = data,
        timestamp = os.time()
    }
end

function clear_cache()
    cache = {}
end
