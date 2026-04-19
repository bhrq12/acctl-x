# acctl

AC Controller for OpenWrt — manage enterprise WiFi Access Points centrally.

## Features

- **Centralized AC Server** (acser): manages APs via TCP control channel + Ethernet broadcast discovery
- **Lightweight AP Client** (apctl): runs on managed APs, auto-registers to AC, reports status periodically
- **CHAP Authentication**: shared secret stored in UCI config (`/etc/config/acctl`), no hardcoded passwords
- **JSON File Database**: zero-dependency JSON database (`/etc/acctl/ac.json`) using libjson-c; no SQLite
- **AP Grouping & Batch Config**: assign APs to groups, push SSID/channel/power settings in bulk
- **Alarm/Event System**: threshold-based alarms (AP offline, weak signal, CPU/memory overload, etc.)
- **Firmware OTA**: push firmware upgrades to managed APs over the control channel
- **Multi-SSID Management**: per-AP SSID templates with 2.4GHz/5GHz band separation
- **LuCI Web UI**: full management panel for AC config, AP list, groups, alarms, and firmware

## Architecture

```
+----------------+     TCP (port 7960)     +----------------+
|  AC Server    | <--------------------> |   AP Client    |
|   (acser)     |     ETH Broadcast       |   (apctl)     |
+----------------+  (discovery probe)      +----------------+
       |                                         |
   /etc/acctl/                              broadcasts
   ac.json (JSON DB)                       registration
       |
  LuCI Web UI
```

- **acser**: AC Controller daemon — runs on OpenWrt router acting as the WiFi controller
- **apctl**: AP Client daemon — runs on each managed Access Point hardware

## Build

Requires OpenWrt SDK or full buildroot with Lean/lede feeds.

```bash
# Copy package into OpenWrt build tree
cd /path/to/openwrt
cp -r acctl package/

# Install dependencies from feeds
./scripts/feeds install libuci-lua

# Compile
make package/acctl/compile V=99
```

## Install

The build produces `acctl_*.ipk`. Install on the AC router:

```bash
opkg install acctl_*.ipk
```

**Installation layout:**
- `/usr/bin/acser` — AC server binary
- `/usr/bin/apctl` — AP client binary
- `/usr/bin/acctl-cli` — CLI management tool
- `/etc/init.d/acctl` — AC server init script (procd)
- `/etc/init.d/apctl` — AP client init script (procd)
- `/etc/config/acctl` — UCI configuration
- `/etc/acctl/ac.json` — JSON database

## Configuration

Configure via UCI. All settings are in `/etc/config/acctl`:

```
# === AC Server ===
config acctl 'acctl'
    option enabled     '1'
    option interface   'br-lan'
    option port        '7960'

    # IMPORTANT: Set a strong password before starting!
    #   uci set acctl.@acctl[0].password='your_secure_password'
    #   uci commit acctl
    option password    ''

    option brditv      '30'       # broadcast probe interval (seconds)
    option reschkitv  '300'      # IP pool reload interval (seconds)
    option msgitv     '3'        # message processing interval (seconds)
    option daemon      '1'        # run as daemon
    option debug       '0'       # debug mode

# === AP Default Profiles ===
config profile 'profile_office'
    option name 'Office Network'
    list ssid_2g 'Office-2G'
    option enc_2g 'psk2'
    option key_2g 'changeme123'
    list ssid_5g 'Office-5G'
    option enc_5g 'psk2'
    option key_5g 'changeme123'

# === Alarm Rules ===
config alarm 'ap_offline'
    option name      'AP Offline'
    option level     '2'
    option threshold '1'
    option window    '60'
    option cooldown  '300'
    option enabled   '1'
```

Access via LuCI: **System → AC Controller**

## Project Structure

```
acctl/
|-- Makefile                 # OpenWrt package Makefile (-DSERVER flag)
|-- src/                     # C source (25 .c files, 21 .h files)
|   |-- include/             # Public headers: aphash, chap, db, dllayer, link,
|   |                       #   log, md5, msg, net, netlayer, sec, sha256, thread
|   |-- ac/                  # AC server (acser) source
|   |   ac.c, net.c, process.c, resource.c, message.c, db.c
|   |   aphash.c (shared AP hash table), cli.c (management CLI)
|   |-- ap/                  # AP client (apctl) source
|   |   main.c, net.c, process.c, message.c, apstatus.c
|   |-- lib/                 # Shared library: epoll, DLL, security, CHAP, JSON
|       arg.c, chap.c, cmdarg.c, dllayer.c, link.c, md5.c, mjson.c
|       netlayer.c, sec.c, sha256.c, thread.c
|-- luci/                    # LuCI web UI
|   |-- applications/luci-app-acctl/
|       controller/          # Lua controller (router registration)
|       model/cbi/acctl/     # CBI models (7 Lua config pages)
|       view/acctl/          # HTML view templates
|       root/etc/uci-defaults/
|-- files/                   # Files installed by ipk
    |-- etc/config/acctl     # UCI configuration template
    |-- etc/init.d/acctl     # AC server init (procd)
    |-- etc/init.d/apctl     # AP client init (procd)
```

## Dependencies

- `libuci-lua` — UCI configuration access
- `libjson-c` — JSON serialization (database)
- `libpthread` — POSIX threads
- `libubus` — OpenWrt system bus (optional)
- `libiwinfo` — wireless device info (optional)

## Supported Platforms

Tested on OpenWrt build targets:

| Target | Architecture | Devices |
|--------|-------------|---------|
| `ipq40xx` | ARM Cortex-A7 | QCN5502/CR660x/X1800/AX3600 |
| `x86_64` | x86_64 | x86 router boards |
| `bcm2711` | ARM Cortex-A72 | Raspberry Pi 4B |

## Security

- CHAP authentication: shared secret from UCI config, never stored in plaintext
- Whitelist-based command validation before dangerous-pattern check
- Single-execution command pattern (no double-execution bugs)
- Per-AP rate limiting: 60 registrations/min, 120 commands/min
- Replay window protection
- HMAC-SHA256 message integrity
- AC trust list for AP authentication

## License

MIT
