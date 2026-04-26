# AC Controller (acctl) v2.0

OpenWrt AC Controller - Centralized Access Point Management System

## Project Structure

This project has been split into three independent packages for better compatibility and flexibility:

### 1. acctl-ac
- **Purpose**: AC Server (acser) - Central controller
- **Main components**: `acser` (AC server), `acctl-cli` (command-line tool)
- **Dependencies**: +libjson-c +libpthread +libubus +libiwinfo
- **Files**: init.d scripts, config files, database

### 2. acctl-ap
- **Purpose**: AP Client (apctl) - Runs on managed APs
- **Main component**: `apctl` (AP client)
- **Dependencies**: +libjson-c +libpthread +libubus +libiwinfo
- **Files**: init.d script for AP client

### 3. luci-app-acctl
- **Purpose**: Web management interface
- **Compatibility**: OpenWrt 18.06-21.02 (Lua) and OpenWrt 22+ (ucode)
- **Dependencies**: +acctl-ac +libuci-lua +libubus-lua
- **Files**: LuCI controller, views, CBI models

## Build Instructions

### Method 1: Build individual packages

```bash
# Build AC server package
make package/acctl/Makefile.ac compile V=99

# Build AP client package
make package/acctl/Makefile.ap compile V=99

# Build LuCI app package
make package/acctl/Makefile.luci compile V=99
```

### Method 2: Build all packages

```bash
# Build all acctl packages
make package/acctl/compile V=99
```

## Installation

### Full installation (recommended)
```bash
opkg install acctl-ac acctl-ap luci-app-acctl
```

### Minimal installation
```bash
# For AC server only
opkg install acctl-ac

# For AP client only
opkg install acctl-ap
```

## Usage

### AC Server (acser)
- **Service management**: `/etc/init.d/acctl start|stop|restart|status`
- **Command-line tool**: `acctl-cli`
- **Web interface**: `http://<device-ip>/cgi-bin/luci/admin/network/acctl`

### AP Client (apctl)
- **Service management**: `/etc/init.d/apctl start|stop|restart|status`

## Configuration

### AC Server
- **Main config**: `/etc/config/acctl`
- **Database**: `/etc/acctl/ac.json`
- **Default password**: `acctl@2024`

### AP Client
- **Configured automatically** by AC server

## Compatibility

- **OpenWrt 18.06-21.02**: Uses Lua-based LuCI
- **OpenWrt 22+**: Uses ucode-based LuCI
- **Architecture**: Compatible with all OpenWrt-supported architectures

## Features

- **Centralized AP management** via TCP + ETH broadcast
- **CHAP authentication** with UCI-stored passwords
- **JSON file-based database** for configuration and status
- **AP grouping** and batch configuration
- **Alarm/event logging** with severity levels
- **Firmware OTA upgrade** support
- **Multi-SSID** support
- **Profile-based configuration** templates
- **Real-time status monitoring**

## Troubleshooting

### Common issues
1. **Service won't start**: Check `/etc/config/acctl` for valid configuration
2. **APs not discovered**: Ensure APs are running `apctl` and on the same network
3. **Web interface error**: Clear browser cache or try a different browser
4. **Database issues**: Check permissions on `/etc/acctl/ac.json`

### Logs
- **System logs**: `logread | grep acctl`
- **Service status**: `/etc/init.d/acctl status`

## Security

- **Password protection**: Change default password via LuCI or UCI
- **CHAP authentication**: Secure AP registration
- **Rate limiting**: Protection against brute force attacks
- **Input validation**: Sanitized user inputs

## License

Apache-2.0

## Author

jianxi sun <ycsunjane@gmail.com>

## Repository

https://github.com/bhrq12/acctl