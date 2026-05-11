module("acctl.middleware", package.seeall)

local http = require "luci.http"
local sys = require "luci.sys"
local uci = require "luci.model.uci".cursor()

function generate_csrf_token()
    local token = http.getcookie("csrf_token")
    if not token or token == "" then
        -- Generate random token using urandom
        local urandom = io.open("/dev/urandom", "rb")
        if urandom then
            local rand_bytes = urandom:read(16)
            urandom:close()
            token = ""
            for i = 1, 16 do
                token = token .. string.format("%02x", string.byte(rand_bytes, i))
            end
        else
            -- Fallback to timestamp based token
            token = string.format("%x%x", os.time(), sys.uptime() * 1000)
        end
        http.setcookie("csrf_token", token, { path = "/", httponly = true })
    end
    return token
end

function check_csrf()
    local token = http.formvalue("csrf_token") or http.getenv("HTTP_X_CSRF_TOKEN")
    local session_token = http.getcookie("csrf_token")
    
    if not token or not session_token or token ~= session_token then
        sys.log("ACCTL", "CSRF token validation failed")
        http.status(403, "Forbidden")
        http.prepare_content("application/json")
        http.write('{"code": 403, "message": "CSRF token validation failed"}')
        return false
    end
    return true
end

function sanitize_input(input)
    if not input then return nil end
    local safe = input:gsub("[<>&\"'%;|`$()]", "")
    return safe
end

function escape_html(input)
    if not input then return "" end
    return tostring(input)
        :gsub("&", "&amp;")
        :gsub("<", "&lt;")
        :gsub(">", "&gt;")
        :gsub('"', "&quot;")
        :gsub("'", "&#039;")
end

function check_permission(required_perm)
    local user = http.getenv("REMOTE_USER")
    if not user or user == "" then
        http.status(401, "Unauthorized")
        http.prepare_content("application/json")
        http.write('{"code": 401, "message": "Authentication required"}')
        return false
    end
    return true
end
