# AC Controller (acctl) v2.0

OpenWrt AC 控制器 - 集中式接入点管理系统

## 项目结构

本项目已拆分为三个独立的软件包，以提高兼容性和灵活性：

### 1. acctl-ac
- **用途**: AC 服务器 (acser) - 中央控制器
- **主要组件**: `acser` (AC 服务器), `acctl-cli` (命令行工具)
- **依赖**: +libjson-c +libpthread +libubus +libiwinfo +libmicrohttpd +libopenssl
- **文件**: init.d 脚本、配置文件、数据库

### 2. acctl-ap
- **用途**: AP 客户端 (apctl) - 在受管 AP 上运行
- **主要组件**: `apctl` (AP 客户端)
- **依赖**: +libjson-c +libpthread +libubus +libiwinfo +libopenssl
- **文件**: AP 客户端的 init.d 脚本

### 3. luci-app-acctl
- **用途**: Web 管理界面
- **兼容性**: OpenWrt 18.06-21.02 (Lua) 和 OpenWrt 22+ (ucode)
- **依赖**: +acctl-ac +libuci-lua +libubus-lua
- **文件**: LuCI 控制器、视图、CBI 模型

## 编译说明

### 方法一：编译单个软件包

```bash
# 编译 AC 服务器软件包
make package/acctl/Makefile.ac compile V=99

# 编译 AP 客户端软件包
make package/acctl/Makefile.ap compile V=99

# 编译 LuCI 应用软件包
make package/acctl/Makefile.luci compile V=99
```

### 方法二：编译所有软件包

```bash
# 编译所有 acctl 软件包
make package/acctl/compile V=99
```

## 安装

### 完整安装（推荐）
```bash
opkg install acctl-ac acctl-ap luci-app-acctl
```

### 最小安装
```bash
# 仅安装 AC 服务器
opkg install acctl-ac

# 仅安装 AP 客户端
opkg install acctl-ap
```

## 使用方法

### AC 服务器 (acser)
- **服务管理**: `/etc/init.d/acctl-ac start|stop|restart|status`
- **命令行工具**: `acctl-cli`
- **Web 界面**: `http://<设备IP>/cgi-bin/luci/admin/network/acctl`

### AP 客户端 (apctl)
- **服务管理**: `/etc/init.d/apctl start|stop|restart|status`

## 配置

### AC 服务器
- **主配置文件**: `/etc/config/acctl-ac`
- **数据库**: `/etc/acctl-ac/ac.json`
- **注意**: 首次部署时必须配置密码

### AP 客户端
- **自动配置** - 由 AC 服务器自动配置

## 兼容性

- **OpenWrt 18.06-21.02**: 使用基于 Lua 的 LuCI
- **OpenWrt 22+**: 使用基于 ucode 的 LuCI
- **架构**: 兼容所有 OpenWrt 支持的架构（ARM、ARM64、x86_64、MIPS 等）

## 功能特性

- **集中式 AP 管理** - 通过 TCP + ETH 广播实现
- **CHAP 认证** - 使用 UCI 存储密码
- **JSON 文件数据库** - 用于配置和状态存储
- **AP 分组管理** - 支持批量配置
- **告警/事件日志** - 带严重级别
- **固件 OTA 升级** - 支持远程升级
- **多 SSID 支持** - 配置多个无线网络
- **基于配置文件的模板** - 快速部署配置
- **实时状态监控** - 实时查看 AP 状态

## 故障排除

### 常见问题
1. **服务无法启动**: 检查 `/etc/config/acctl-ac` 配置文件是否有效
2. **AP 无法发现**: 确保 AP 正在运行 `apctl` 且在同一网络中
3. **Web 界面错误**: 清除浏览器缓存或尝试其他浏览器
4. **数据库问题**: 检查 `/etc/acctl-ac/ac.json` 的权限

### 日志查看
- **系统日志**: `logread | grep acctl`
- **服务状态**: `/etc/init.d/acctl-ac status`

## 安全

- **密码保护**: 通过 LuCI 或 UCI 更改默认密码
- **CHAP 认证**: 安全的 AP 注册机制
- **速率限制**: 防止暴力破解攻击
- **输入验证**: 用户输入已进行安全清理
- **基于令牌的 API 认证**: 使用限时令牌保护 API 访问

**重要提示**: 首次安装后，必须设置强密码：

```bash
uci set acctl-ac.@acctl[0].password='your_secure_password'
uci commit acctl-ac
/etc/init.d/acctl-ac restart
```

## CAPWAP 协议支持

本项目支持 CAPWAP 协议实现：

- **控制通道**: UDP 5246 端口
- **数据通道**: UDP 5247 端口
- **DTLS 加密**: 可选的安全通信
- **消息类型**: Discovery、Join、Echo、Configure、Reset、Statistics、Image Data
- **状态机**: IDLE → DISCOVERY → JOIN → CONFIGURE → RUN

## 许可证

Apache-2.0

详细信息请参阅 [LICENSE](LICENSE) 和 [THIRD-PARTY-LICENSES.md](THIRD-PARTY-LICENSES.md)。

## 作者

jianxi sun <ycsunjane@gmail.com>

## 代码仓库

https://github.com/bhrq12/acctl

---

*最后更新: 2026-04-29*
