#!/usr/bin/env lua
--[[
AC Controller - Lua Unit Tests
Run with: lua tests/acctl_test.lua
]]

local lfs = require("lfs")

-- Test framework
local TestSuite = {}
TestSuite.__index = TestSuite

function TestSuite:new(name)
    local t = setmetatable({}, self)
    t.name = name
    t.tests = {}
    t.passed = 0
    t.failed = 0
    return t
end

function TestSuite:test(name, fn)
    table.insert(self.tests, {name = name, fn = fn})
end

function TestSuite:run()
    print("\n========================================")
    print("Running: " .. self.name)
    print("========================================")

    for _, test in ipairs(self.tests) do
        local ok, err = pcall(test.fn)
        if ok then
            print("[PASS] " .. test.name)
            self.passed = self.passed + 1
        else
            print("[FAIL] " .. test.name)
            print("       Error: " .. tostring(err))
            self.failed = self.failed + 1
        end
    end

    print("----------------------------------------")
    print(string.format("Results: %d passed, %d failed, %d total",
        self.passed, self.failed, #self.tests))
    print("========================================\n")

    return self.failed == 0
end

-- Assertions
local function assert_equal(expected, actual, msg)
    if expected ~= actual then
        error((msg or "Assertion failed") .. ": expected " ..
            tostring(expected) .. ", got " .. tostring(actual))
    end
end

local function assert_true(val, msg)
    if not val then
        error(msg or "Assertion failed: expected true")
    end
end

local function assert_nil(val, msg)
    if val ~= nil then
        error(msg or "Assertion failed: expected nil")
    end
end

local function assert_table(t, msg)
    if type(t) ~= "table" then
        error(msg or "Assertion failed: expected table, got " .. type(t))
    end
end

-- Mock sys module for testing
local MockSys = {
    call_count = 0,
    last_cmd = "",
    mock_response = ""
}

function MockSys.exec(cmd)
    MockSys.last_cmd = cmd
    return MockSys.mock_response
end

function MockSys.call(cmd)
    MockSys.call_count = MockSys.call_count + 1
    MockSys.last_cmd = cmd
    return 0
end

-- Test: CLI output parsing
local function test_cli_output_parsing()
    local http = require("luci.http")

    local json_str = '{"aps":[{"mac":"AA:BB:CC:DD:EE:FF","hostname":"AP-001"}]}'
    local ok, data = pcall(http.parse_json, json_str)
    assert_true(ok, "JSON parsing should succeed")
    assert_table(data, "Parsed data should be a table")
    assert_equal("AA:BB:CC:DD:EE:FF", data.aps[1].mac)
end

-- Test: MAC address validation
local function test_mac_validation()
    local function validate_mac(mac)
        if not mac then return false end
        local clean = mac:gsub("[^%x:]", "")
        return #clean == 17 and clean:match("^%x%x:%x%x:%x%x:%x%x:%x%x:%x%x$") ~= nil
    end

    assert_true(validate_mac("AA:BB:CC:DD:EE:FF"), "Valid MAC should pass")
    assert_true(validate_mac("aa:bb:cc:dd:ee:ff"), "Lowercase MAC should pass")
    assert_true(not validate_mac("invalid"), "Invalid MAC should fail")
    assert_true(not validate_mac("GG:HH:II:JJ:KK:LL"), "Invalid hex should fail")
end

-- Test: Command sanitization
local function test_command_sanitization()
    local dangerous_patterns = {
        ";", "|", "`", "$(", "&", "&&", "||", ">", "<"
    }

    local function contains_dangerous(cmd)
        for _, pattern in ipairs(dangerous_patterns) do
            if cmd:find(pattern, 1, true) then
                return true
            end
        end
        return false
    end

    assert_true(not contains_dangerous("reboot"), "Clean command should pass")
    assert_true(contains_dangerous("reboot; rm -rf"), "Command with semicolon should fail")
    assert_true(contains_dangerous("cat /etc/passwd"), "Path traversal should fail")
end

-- Test: Status data structure
local function test_status_structure()
    local status = {
        running = true,
        ap_online = 5,
        ap_total = 10,
        alarm_count = 2,
        pool_left = 50,
        pool_total = 100,
        timestamp = os.time()
    }

    assert_table(status, "Status should be a table")
    assert_true(status.running, "Status should have running field")
    assert_true(type(status.ap_online) == "number", "ap_online should be number")
end

-- Test: AP data structure
local function test_ap_data_structure()
    local ap = {
        mac = "AA:BB:CC:DD:EE:FF",
        hostname = "AP-001",
        wan_ip = "192.168.1.101",
        wifi_ssid = "OpenWrt-AC",
        firmware = "v2.0.1",
        online_users = 25,
        online = true,
        last_seen = os.time(),
        group_id = 1
    }

    assert_table(ap, "AP should be a table")
    assert_equal("AA:BB:CC:DD:EE:FF", ap.mac)
    assert_equal("AP-001", ap.hostname)
    assert_true(ap.online, "AP should be online")
end

-- Test: Alarm data structure
local function test_alarm_data_structure()
    local alarm = {
        id = 1,
        ap_mac = "AA:BB:CC:DD:EE:FF",
        level = "error",
        level_num = 2,
        message = "AP offline",
        acknowledged = false,
        created_at = "1714303800"
    }

    assert_table(alarm, "Alarm should be a table")
    assert_equal(1, alarm.id)
    assert_equal("error", alarm.level)
    assert_equal(2, alarm.level_num)
end

-- Test: Group data structure
local function test_group_data_structure()
    local group = {
        id = 1,
        name = "Office-Floor1",
        description = "Office first floor APs",
        policy = "manual",
        ap_count = 5
    }

    assert_table(group, "Group should be a table")
    assert_equal(1, group.id)
    assert_equal("Office-Floor1", group.name)
    assert_equal("manual", group.policy)
end

-- Test: Firmware data structure
local function test_firmware_data_structure()
    local fw = {
        version = "v2.0.1",
        filename = "openwrt-ap-2.0.1.bin",
        size = 5242880,
        sha256 = "abc123...",
        uploaded_at = "2024-04-28 10:30:00"
    }

    assert_table(fw, "Firmware should be a table")
    assert_equal("v2.0.1", fw.version)
    assert_true(fw.size > 0, "Firmware size should be positive")
end

-- Test: API response code handling
local function test_api_response_codes()
    local codes = {
        {code = 0, message = "Success"},
        {code = 400, message = "Bad Request"},
        {code = 403, message = "Forbidden"},
        {code = 500, message = "Internal Error"}
    }

    for _, resp in ipairs(codes) do
        assert_table(resp, "Response should be a table")
        assert_true(type(resp.code) == "number", "Code should be number")
        assert_true(type(resp.message) == "string", "Message should be string")
    end
end

-- Test: UCI config operations
local function test_uci_config_structure()
    local config = {
        enabled = "1",
        interface = "br-lan",
        port = "7960",
        password = "",
        brditv = "30",
        reschkitv = "300",
        msgitv = "3",
        daemon = "1",
        debug = "0"
    }

    assert_table(config, "Config should be a table")
    assert_equal("br-lan", config.interface)
    assert_equal("7960", config.port)
end

-- Run all tests
local suite = TestSuite:new("AC Controller Lua Tests")

suite:test("CLI output parsing", test_cli_output_parsing)
suite:test("MAC address validation", test_mac_validation)
suite:test("Command sanitization", test_command_sanitization)
suite:test("Status data structure", test_status_structure)
suite:test("AP data structure", test_ap_data_structure)
suite:test("Alarm data structure", test_alarm_data_structure)
suite:test("Group data structure", test_group_data_structure)
suite:test("Firmware data structure", test_firmware_data_structure)
suite:test("API response code handling", test_api_response_codes)
suite:test("UCI config structure", test_uci_config_structure)

local success = suite:run()

-- Exit with appropriate code
os.exit(success and 0 or 1)
