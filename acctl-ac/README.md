# acctl-ac

AC Controller Server with LuCI Web Management Interface

## Overview

acctl-ac is the central controller component of the AC Controller system. It runs on the main controller device and provides:

- **AC Server Daemon (acser)**: Central AP management via CAPWAP protocol
- **REST API**: HTTP API for third-party integrations
- **LuCI Web Interface**: Web-based configuration and monitoring
- **CLI Tool (acctl-cli)**: Command-line management interface

## Architecture

```
┌─────────────────────────────────────────────┐
│              acctl-ac                        │
├─────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────────────┐  │
│  │  acser     │  │    LuCI Web UI     │  │
│  │  (Daemon)  │  │   (JavaScript)     │  │
│  │            │  │                    │  │
│  │ - CAPWAP   │  │ - AP Management   │  │
│  │ - HTTP API │  │ - Configuration   │  │
│  │ - Database │  │ - Monitoring       │  │
│  │ - AP Mgmt  │  │ - Firmware Mgmt   │  │
│  └──────┬──────┘  └─────────┬──────────┘  │
│         │                    │             │
│         │     HTTP/REST      │             │
│         └──────────┬─────────┘             │
│                    │                       │
│  ┌─────────────────┴─────────────────────┐ │
│  │          CAPWAP Protocol              │ │
│  │   (UDP 5246/5247)                   │ │
│  └─────────────────┬─────────────────────┘ │
└────────────────────┼───────────────────────┘
                     │
                     ▼
              ┌──────────────┐
              │  acctl-ap   │
              │ (AP Client) │
              └──────────────┘
```

## Features

### Core Server Features
- **CAPWAP Protocol**: Standards-based AP management
- **Multi-AP Support**: Manage up to 100+ APs simultaneously
- **AP Discovery**: Automatic AP discovery via broadcast
- **Configuration Push**: Remote AP configuration
- **Firmware OTA**: Over-the-air firmware upgrades
- **Alarm Management**: Event logging and alerting
- **IP Pool Management**: Automatic IP address allocation

### Web Interface Features
- **Dashboard**: Real-time system status
- **AP Management**: View and control all APs
- **Configuration**: Configure APs and groups
- **Firmware Management**: Upload and deploy firmware
- **Alarm Viewer**: View and acknowledge alarms
- **Responsive Design**: Mobile-friendly interface

### API Features
- **RESTful Design**: Standard HTTP API
- **Token Authentication**: Secure API access
- **JSON Format**: Easy integration with other systems

## Installation

### From OpenWrt Package Feed

```bash
opkg update
opkg install acctl-ac
```

### Manual Installation

```bash
# Copy the ipk file to your OpenWrt device
scp acctl-ac_2.0-1_arm_cortex-a7_neon-vfpv4.ipk root@192.168.1.1:/tmp/

# Install the package
ssh root@192.168.1.1
opkg install /tmp/acctl-ac_2.0-1_arm_cortex-a7_neon-vfpv4.ipk
```

## Configuration

### UCI Configuration

Edit `/etc/config/acctl-ac`:

```uci
config acctl-ac 'global'
    option enabled '1'
    option name 'My-AC-Controller'

config acctl-ac 'network'
    option listen_port '8080'
    option capwap_port '5246'
    list nic 'br-lan'

config acctl-ac 'resource'
    option ip_start '192.168.1.200'
    option ip_end '192.168.1.254'
    option ip_mask '255.255.255.0'

config acctl-ac 'security'
    option password 'your_secure_password'
    option dtls_enabled '0'
```

### Initial Setup

1. Access LuCI interface at `http://<device-ip>/cgi-bin/luci/admin/network/acctl`
2. Login with your OpenWrt root password
3. Configure AC settings in Settings tab
4. Enable APs to connect

## Usage

### Daemon Control

```bash
# Start the service
/etc/init.d/acctl-ac start

# Stop the service
/etc/init.d/acctl-ac stop

# Restart the service
/etc/init.d/acctl-ac restart

# Enable auto-start
/etc/init.d/acctl-ac enable
```

### CLI Tool

```bash
# Show AC status
acctl-cli status

# List connected APs
acctl-cli ap list

# Get AP details
acctl-cli ap show <ap-mac>

# Reboot an AP
acctl-cli ap reboot <ap-mac>

# View alarms
acctl-cli alarm list
```

### HTTP API

```bash
# Login and get token
curl -X POST http://localhost:8080/api/v1/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"root","password":"your_password"}'

# List APs
curl http://localhost:8080/api/v1/aps \
  -H "Authorization: Bearer <token>"

# Get AP details
curl http://localhost:8080/api/v1/aps/<mac> \
  -H "Authorization: Bearer <token>"
```

## Deployment Modes

### Combined Mode (Same Device)

Deploy both acctl-ac and acctl-ap on the same device:

```bash
opkg install acctl-ac acctl-ap
```

Both components communicate via localhost.

### Distributed Mode (Separate Devices)

Deploy acctl-ac on a dedicated controller device:

```bash
opkg install acctl-ac
```

Deploy acctl-ap on each managed AP device:

```bash
opkg install acctl-ap
```

Configure apctl to connect to the AC:

```bash
uci set acctl-ap.global.ac_ip='192.168.1.100'
uci commit acctl-ap
/etc/init.d/acctl-ap restart
```

## Development

### Build from Source

```bash
# Clone the repository
git clone https://github.com/bhrq12/acctl-ac.git
cd acctl-ac

# Install build dependencies (on Ubuntu/Debian)
sudo apt-get build-dep gcc make libc-dev

# Build for OpenWrt (requires OpenWrt SDK)
export STAGING_DIR=/path/to/openwrt/staging_dir
make

# Build unit tests
cd test/unit
make
./test_capwap
```

### Testing

```bash
# Run unit tests
make test

# Run integration tests
make test-integration

# Run all tests with coverage
make test-all
```

## Protocol

acctl-ac uses the CAPWAP protocol for AP communication:

| Port | Protocol | Purpose |
|------|----------|---------|
| 5246/UDP | CAPWAP | Control channel |
| 5247/UDP | CAPWAP | Data channel |
| 8080/TCP | HTTP | REST API |

See [CAPWAP Protocol Specification](../../protocol/capwap_protocol.h) for details.

## Security

### Authentication

- CLI: Use OpenWrt root password
- LuCI: Use LuCI authentication
- API: Token-based authentication with expiry

### Recommendations

1. **Change Default Password**: Set a strong password in UCI config
2. **Enable DTLS**: Enable DTLS encryption for production
3. **Firewall**: Restrict API access to trusted networks
4. **Updates**: Keep firmware updated

## Troubleshooting

### AP Not Connecting

```bash
# Check if acser is running
ps | grep acser

# Check CAPWAP port
netstat -ulnp | grep 5246

# Check firewall
iptables -L -n | grep 5246
```

### Database Issues

```bash
# Check database file
ls -la /etc/acctl-ac/ac.json

# Validate JSON
jsonlint /etc/acctl-ac/ac.json
```

### View Logs

```bash
# System log
logread | grep acser

# Direct output (if not daemonized)
acser -d
```

## License

See [LICENSE](../../LICENSE) file.

## Maintainer

jianxi sun <ycsunjane@gmail.com>

## Contributing

See [CONTRIBUTING.md](../../CONTRIBUTING.md) for development guidelines.
