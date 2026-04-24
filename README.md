# AC Controller Project

## 项目概述

AC Controller 是一个用于管理接入控制器(AC)和接入点(AP)的系统，版本为 2.0。该项目旨在为OpenWrt系统提供一个集中管理无线接入点的解决方案，支持AP的发现、配置、监控和固件升级等功能。

## 功能特性

- **AC服务器(acser)**：通过TCP和以太网广播集中管理AP
- **AP客户端(apctl)**：运行在每个被管理的接入点上
- **CHAP认证**：使用UCI存储的密码进行认证（无硬编码密码）
- **JSON文件数据库**：用于存储AP配置和状态
- **AP分组和批量配置**：支持按组管理AP并进行批量配置
- **告警/事件日志**：记录系统事件和告警信息
- **固件OTA升级**：支持远程固件升级
- **LuCI Web管理界面**：提供友好的Web管理界面

## 系统架构

### 系统组成
AC Controller系统由以下组件组成：

```
┌─────────────────────────────────────────────────────────┐
│                    AC Controller                      │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐        │
│  │  acser │    │acctl-cli│    │  LuCI  │        │
│  │  (Server)│    │  (CLI)   │    │  (Web) │        │
│  └────┬───┘    └────┬─────┘    └────┬─────┘        │
│       │               │               │               │
│       └──────────┬──────────┘               │               │
│                  │                     │               │
│           ┌───────▼───────┐             │               │
│           │   JSON DB     │◄────────────┘               │
│           └───────────────────┘                             │
└───────────────────────┬─────────────────────────────────┘
                        │
           ┌────────────┼─────────────┐
           │            │             │
┌──────────▼───┐  ┌──────▼──────┐ ┌──────▼───────┐
│     AP 1   │  │     AP 2    │ │     AP 3    │
│   (apctl)    │  │   (apctl)    │ │   (apctl)    │
└───────────────┘  └───────────────┘ └───────────────┘
```

### 系统组件

1. **acser** (AC服务器)
   - 管理接入点(AP)的中央控制器
   - TCP端口：接收和处理AP注册、配置和监控
   - 管理IP地址池
   - 记录事件和告警
   - 运行在OpenWrt设备上

2. **apctl** (AP客户端)
   - 运行在每个被管理的AP上
   - 与AC服务器通信
   - 执行配置更新
   - 报告状态信息

3. **acctl-cli** (命令行工具)
   - 管理和监控AC控制器
   - 执行AP操作：重启、配置、升级等
   - 查询状态

4. **LuCI Web界面**
   - 友好的图形化管理界面
   - 集成到OpenWrt管理系统

### 通信协议

- **TCP (7960端口)：AC和AP之间的通信
- **Eth广播**：AP发现和注册
- **JSON**：数据交换格式

### 数据存储

- JSON文件数据库
- 位置：/etc/acctl/ac.json
- 自动备份文件：/etc/acctl/ac.json.backup

## 系统要求

- OpenWrt 18.06 或更高版本
- 至少 16MB 可用内存
- 至少 1MB 可用存储空间
- 支持的无线接入点设备

## 安装方法

### 从源码编译

1. **准备OpenWrt构建环境**
   ```bash
   git clone https://git.openwrt.org/openwrt/openwrt.git
   cd openwrt
   ./scripts/feeds update -a
   ./scripts/feeds install -a
   ```

2. **添加AC Controller包**
   ```bash
   mkdir -p package/network
   cd package/network
   git clone https://github.com/bhrq12/acctl.git
   cd ../..
   ```

3. **配置构建选项**
   ```bash
   make menuconfig
   ```
   在 `Network -> Access Controllers` 中选择 `acctl` 包

4. **编译包**
   ```bash
   make package/acctl/compile V=s
   ```

5. **安装生成的IPK包**
   ```bash
   opkg install bin/packages/*/acctl_*.ipk
   ```

### 直接安装预编译包

如果有预编译的IPK包，可以直接使用opkg安装：

```bash
opkg install acctl_*.ipk
```

## 配置说明

### 基本配置

AC Controller的配置文件位于 `/etc/config/acctl`，主要配置选项包括：

```lua
config acctl 'acctl'
	option enabled     '1'     # 是否启用AC控制器
	option interface   'br-lan' # 监听接口
	option port        '7960'  # TCP端口
	option password    ''      # 认证密码（必须设置）
	option brditv      '30'    # 广播间隔（秒）
	option reschkitv   '300'   # IP池重载间隔（秒）
	option msgitv      '3'     # 消息处理间隔（秒）
	option daemon      '1'     # 以守护进程运行
	option debug       '0'     # 调试模式
	option ap_enabled  '0'     # AP客户端模式
	option multi_ssid  '1'     # 多SSID支持
	option default_band 'auto'  # 默认频段
```

### 密码设置

首次安装后，必须设置密码：

```bash
uci set acctl.@acctl[0].password='your_secure_password'
uci commit acctl
```

### IP地址池配置

IP地址池通过JSON数据库配置，编辑 `/etc/acctl/ac.json`：

```json
{
  "resource": {
    "ip_start": "192.168.100.1",
    "ip_end": "192.168.100.254",
    "ip_mask": "255.255.255.0"
  }
}
```

### AP默认配置模板

可以配置多个AP配置模板：

```lua
config profile 'profile_office'
	option name     'Office Network'
	option desc     'Corporate office WiFi configuration'

	# Primary 2.4GHz SSID
	list ssid_2g    'Office-2G'
	option enc_2g   'psk2'
	option key_2g   'changeme123'
	option channel_2g 'auto'
	option mode_2g  'n'
	option power_2g 'auto'

	# Primary 5GHz SSID
	list ssid_5g    'Office-5G'
	option enc_5g   'psk2'
	option key_5g   'changeme123'
	option channel_5g 'auto'
	option mode_5g  'ac'
	option power_5g 'auto'
```

## 使用指南

### 启动/停止服务

```bash
# 启动服务
/etc/init.d/acctl start

# 停止服务
/etc/init.d/acctl stop

# 重启服务
/etc/init.d/acctl restart

# 设置开机自启
/etc/init.d/acctl enable

# 禁用开机自启
/etc/init.d/acctl disable
```

### Web界面访问

在浏览器中访问 `http://<router-ip>/cgi-bin/luci/admin/network/acctl`，使用OpenWrt的管理员账号登录。

### 命令行工具

AC Controller提供了命令行工具 `acctl-cli`，用于管理和监控系统：

```bash
# 查看系统状态
acctl-cli stats

# 列出所有AP
acctl-cli aps

# 列出所有告警
acctl-cli alarms

# 重启指定AP
acctl-cli reboot <mac>

# 推送配置到指定AP
acctl-cli config <mac>

# 升级指定AP的固件
acctl-cli upgrade <mac> <firmware>
```

## 安全注意事项

1. **密码安全**：始终使用强密码，并定期更改
2. **网络安全**：确保AC控制器所在的网络安全，限制访问
3. **固件安全**：只使用官方或可信来源的固件
4. **权限控制**：仅授予必要的权限给管理用户
5. **定期更新**：定期更新AC Controller到最新版本

## 故障排除

### 常见问题

1. **AC控制器无法启动**
   - 检查密码是否设置
   - 检查配置文件是否正确
   - 查看日志：`logread | grep acser`

2. **AP无法发现**
   - 检查AP是否运行apctl
   - 检查网络连接是否正常
   - 检查广播设置是否正确

3. **Web界面无法访问**
   - 检查LuCI是否安装
   - 检查AC控制器是否运行
   - 检查防火墙设置

4. **固件升级失败**
   - 检查固件文件是否正确
   - 检查网络连接是否稳定
   - 检查AP是否有足够的存储空间

### 日志查看

```bash
# 查看AC控制器日志
logread | grep acser

# 查看AP客户端日志
logread | grep apctl

# 查看系统日志
logread
```

## 贡献指南

欢迎贡献代码和改进建议！请按照以下步骤进行：

1. Fork项目仓库
2. 创建特性分支
3. 提交更改
4. 推送到分支
5. 创建Pull Request

## 许可证

本项目采用Apache License 2.0许可证。详见LICENSE文件。

## 联系方式

- 作者：jianxi sun
- 邮箱：ycsunjane@gmail.com
- 项目地址：https://github.com/bhrq12/acctl
