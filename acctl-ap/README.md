# acctl-ap

AP Client for AC Controller System

## Overview

acctl-ap is the lightweight client component that runs on managed Access Points. It connects to the central AC Controller (acctl-ac) and provides:

- **CAPWAP Communication**: Standards-based protocol with AC server
- **Auto-Discovery**: Automatic AC server discovery
- **Status Reporting**: Periodic AP status reports to AC
- **Configuration接收**: Receive and apply configurations from AC
- **Firmware Upgrades**: Execute firmware upgrades from AC

## Architecture

```
┌─────────────────────────────────────────┐
│              acctl-ap                    │
├─────────────────────────────────────────┤
│  ┌─────────────────────────────────────┐│
│  │           apctl (Daemon)           ││
│  │                                     ││
│  │  ┌───────────┐  ┌───────────────┐  ││
│  │  │ CAPWAP   │  │   Wireless    │  ││
│  │  │ Protocol │  │   Manager     │  ││
│  │  └─────┬─────┘  └───────┬───────┘  ││
│  │        │                │           ││
│  │        └────────┬───────┘           ││
│  │                 │                   ││
│  │  ┌──────────────┴──────────────┐    ││
│  │  │      Local Configuration    │    ││
│  │  │    /etc/config/acctl-ap    │    ││
│  │  └─────────────────────────────┘    ││
│  └─────────────────────────────────────┘│
└─────────────────────────────────────────┘
          │
          │ CAPWAP (UDP 5246/5247)
          ▼
┌─────────────────────────────────────────┐
│            acctl-ac                     │
│         (AC Controller)                 │
└─────────────────────────────────────────┘
```

## Features

### Core Features
- **CAPWAP Protocol**: Standards-based AC-AP communication
- **Auto-Discovery**: Discover AC via broadcast or configured IP
- **Secure Channel**: Optional DTLS encryption
- **Heartbeat**: Keepalive with AC server
- **Status Reporting**: CPU, memory, wireless stats
- **Configuration接收**: Apply AC-pushed configurations
- **Firmware Upgrade**: Execute OTA firmware upgrades
- **Local Alarm**: Generate and report local alarms

### Lightweight Design
- **Minimal Dependencies**: Only essential libraries
- **Small Footprint**: Designed for embedded devices
- **Low Resource Usage**: Optimized for MIPS/ARM

## Installation

### From OpenWrt Package Feed

```bash
opkg update
opkg install acctl-ap
```

### Manual Installation

```bash
# Copy the ipk file to your AP device
scp acctl-ap_2.0-1_arm_cortex-a7_neon-vfpv4.ipk root@192.168.1.x:/tmp/

# Install the package
ssh root@192.168.1.x
opkg install /tmp/acctl-ap_2.0-1_arm_cortex-a7_neon-vfpv4.ipk
```

## Configuration

### UCI Configuration

Edit `/etc/config/acctl-ap`:

```uci
config acctl-ap 'global'
    option enabled '1'
    option ac_ip '192.168.1.1'
    option ac_port '5246'
    option heartbeat_interval '30'

config acctl-ap 'device'
    option nic 'br-lan'
    list ssid 'MyNetwork'
    option encryption 'psk2'
    option key 'your_wifi_password'

config acctl-ap 'security'
    option password 'your_ac_password'
```

### Auto-Discovery Mode

To let AP automatically discover AC:

```uci
config acctl-ap 'global'
    option enabled '1'
    option ac_ip '255.255.255.255'
    option ac_port '5246'
    option discovery_mode 'broadcast'
```

### Static AC IP Mode

For fixed AC IP configuration:

```uci
config acctl-ap 'global'
    option enabled '1'
    option ac_ip '192.168.1.100'
    option ac_port '5246'
    option discovery_mode 'static'
```

## Usage

### Daemon Control

```bash
# Start the service
/etc/init.d/acctl-ap start

# Stop the service
/etc/init.d/acctl-ap stop

# Restart the service
/etc/init.d/acctl-ap restart

# Enable auto-start
/etc/init.d/acctl-ap enable

# Check status
/etc/init.d/acctl-ap status
```

### Monitoring

```bash
# Check if apctl is running
ps | grep apctl

# View connection status
cat /etc/acctl-ap/status.json

# Check CAPWAP sockets
netstat -unlp | grep 5246
```

### Debug Mode

```bash
# Run in foreground with debug output
apctl -d

# With specific AC IP
apctl -d -a 192.168.1.100
```

## Deployment Modes

### Combined Mode

Run both acctl-ac and acctl-ap on the same device:

```bash
opkg install acctl-ac acctl-ap
```

The AP client will connect to the local AC server.

### Distributed Mode

Run acctl-ap on dedicated AP devices:

```bash
# On each AP device
opkg install acctl-ap

# Configure AC IP
uci set acctl-ap.global.ac_ip='192.168.1.100'
uci commit acctl-ap
/etc/init.d/acctl-ap start
```

## Protocol

acctl-ap uses the CAPWAP protocol for AC communication:

| Port | Protocol | Purpose |
|------|----------|---------|
| 5246/UDP | CAPWAP | Control channel |
| 5247/UDP | CAPWAP | Data channel |

See [CAPWAP Protocol Specification](../../protocol/capwap_protocol.h) for message format details.

### Message Flow

```
AP Boot → Discovery → Join → Config → Run
                      ↓
              AC sends configuration
              AP applies and acknowledges
                      ↓
              Periodic heartbeat
              Status reports
```

## Troubleshooting

### AP Not Connecting to AC

```bash
# Check configuration
cat /etc/config/acctl-ap

# Verify AC IP is reachable
ping -c 3 192.168.1.100

# Check CAPWAP port is open
netstat -ulnp | grep 5246

# Run in debug mode
apctl -d
```

### Discovery Issues

```bash
# Check if broadcast is working
tcpdump -i any udp port 5246

# Try static AC IP
uci set acctl-ap.global.discovery_mode='static'
uci commit acctl-ap
/etc/init.d/acctl-ap restart
```

### View Logs

```bash
# System log
logread | grep apctl

# Direct output
apctl -d
```

## Protocol Development

### Building Protocol Stack

```bash
# Install OpenWrt SDK
export STAGING_DIR=/path/to/openwrt/staging_dir

# Build the project
cd src
make -f Makefile.ap

# Build unit tests
cd test/unit
make test
```

### Testing

```bash
# Run unit tests
cd test/unit
make test

# Run integration tests
cd test/integration
./test_capwap_flow.sh

# Test with mock AC
cd test/integration
./test_mock_ac.sh
```

## Security

### Recommendations

1. **Use DTLS**: Enable DTLS in production environments
2. **Strong Password**: Use strong passwords in configuration
3. **Network Isolation**: Isolate management network
4. **Firmware Signing**: Verify firmware before upgrade

### DTLS Configuration

```uci
config acctl-ap 'security'
    option password 'your_password'
    option dtls_enabled '1'
    option cert_file '/etc/acctl-ap/cert.pem'
    option key_file '/etc/acctl-ap/key.pem'
```

## Comparison with Original Design

| Aspect | Original | New (acctl-ap) |
|--------|----------|----------------|
| Dependency | acctl-ac required | Independent |
| Communication | Shared lib | Standard Protocol |
| Configuration | Shared UCI | Independent UCI |
| Deployment | Coupled | Fully independent |

## License

See [LICENSE](../../LICENSE) file.

## Maintainer

jianxi sun <ycsunjane@gmail.com>

## Contributing

See [CONTRIBUTING.md](../../CONTRIBUTING.md) for development guidelines.
