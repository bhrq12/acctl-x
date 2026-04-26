# OpenWrt AC Controller (acctl) v2.0

## 项目简介

OpenWrt AC Controller (acctl) 是一个专为 OpenWrt 路由器设计的无线接入点控制器，提供以下功能：

- **集中管理**：统一管理多个无线接入点
- **自动配置**：自动为 AP 分配 IP 地址
- **实时监控**：监控 AP 状态和客户端连接
- **固件管理**：集中管理 AP 固件升级
- **告警系统**：实时告警和事件记录
- **安全认证**：基于 CHAP 的安全认证机制
- **多版本兼容**：支持 OpenWrt 18.06-24.10 版本

## 系统要求

- OpenWrt 18.06 或更高版本
- 至少 64MB 内存
- 至少 10MB 存储空间
- 网络接口（推荐使用 br-lan）

## 安装方法

### 方法一：使用 opkg 安装

1. 下载适合您设备架构的 acctl 软件包
2. 使用 opkg 命令安装：
   ```bash
   opkg install acctl_2.0-1_*.ipk
   ```

### 方法二：从源代码编译

1. 克隆源代码仓库：
   ```bash
   git clone https://github.com/yourusername/acctl.git
   ```

2. 将 acctl 目录复制到 OpenWrt 构建系统的 package 目录：
   ```bash
   cp -r acctl /path/to/openwrt/package/
   ```

3. 配置构建选项：
   ```bash
   make menuconfig
   ```
   在 "Network" 菜单中选择 "acctl"

4. 编译软件包：
   ```bash
   make package/acctl/compile V=99
   ```

5. 安装生成的软件包：
   ```bash
   opkg install bin/packages/*/base/acctl_2.0-1_*.ipk
   ```

## 自动配置

安装完成后，acctl 会自动进行以下配置：

1. **默认密码**：设置为 `acctl@2024`
2. **IP 地址池**：默认分配 192.168.1.200-192.168.1.254
3. **服务启动**：自动启用并启动服务
4. **数据库初始化**：创建默认的 JSON 数据库

## Web 界面访问

1. 打开浏览器，访问 OpenWrt Web 管理界面
2. 导航到 "网络" → "AC Controller"
3. 使用默认密码 `acctl@2024` 登录

## 命令行工具

acctl 提供了命令行工具 `acctl-cli`，用于管理和监控 AP：

```bash
# 列出所有 AP
tcctl-cli aps

# 列出所有告警
acctl-cli alarms

# 列出所有固件
acctl-cli firmware

# 查看数据库统计信息
acctl-cli stats

# 确认告警
acctl-cli ack <alarm_id>

# 确认所有告警
acctl-cli ack-all

# 写入审计日志
acctl-cli audit <user> <action> <rtype> <rid> <old> <new> <ip>
```

## 配置文件

### 主要配置文件

- **/etc/config/acctl**：UCI 配置文件，包含基本设置
- **/etc/acctl/ac.json**：JSON 格式的数据库文件

### UCI 配置选项

```bash
# 编辑配置文件
uci edit acctl

# 查看配置
uci show acctl

# 提交更改
uci commit acctl
```

主要配置选项：

| 选项 | 说明 | 默认值 |
|------|------|--------|
| enabled | 是否启用服务 | 1 (启用) |
| interface | 网络接口 | br-lan |
| port | 服务端口 | 7960 |
| password | 管理密码 | acctl@2024 |
| brditv | 广播间隔 (秒) | 30 |
| reschkitv | IP 池刷新间隔 (秒) | 300 |
| msgitv | 消息处理间隔 (秒) | 3 |
| daemon | 守护进程模式 | 1 (启用) |
| debug | 调试模式 | 0 (禁用) |

## 服务管理

```bash
# 启动服务
/etc/init.d/acctl start

# 停止服务
/etc/init.d/acctl stop

# 重启服务
/etc/init.d/acctl restart

# 启用服务（开机自启）
/etc/init.d/acctl enable

# 禁用服务
/etc/init.d/acctl disable

# 查看服务状态
/etc/init.d/acctl status
```

## 故障排查

### 常见问题

1. **服务无法启动**
   - 检查配置文件是否正确
   - 检查端口是否被占用
   - 查看系统日志：`logread | grep acctl`

2. **Web 界面无法访问**
   - 检查服务是否运行：`/etc/init.d/acctl status`
   - 检查防火墙设置
   - 清除浏览器缓存

3. **AP 无法注册**
   - 检查 AP 是否在同一网络
   - 检查密码是否正确
   - 查看 AC 控制器日志

### 日志查看

```bash
# 查看系统日志中的 acctl 相关信息
logread | grep acctl

# 查看详细日志（启用调试模式后）
logread -f | grep acctl
```

## 安全建议

1. **更改默认密码**：安装后请立即更改默认密码
   ```bash
   uci set acctl.@acctl[0].password='your_secure_password'
   uci commit acctl
   /etc/init.d/acctl restart
   ```

2. **限制访问**：通过防火墙限制对 AC 控制器的访问

3. **定期更新**：定期更新 acctl 到最新版本

## 版本兼容性

| OpenWrt 版本 | 支持状态 | LuCI 类型 |
|-------------|----------|-----------|
| 18.06 | 支持 | Lua |
| 19.07 | 支持 | Lua |
| 21.02 | 支持 | Lua |
| 22.03 | 支持 | ucode |
| 23.05 | 支持 | ucode |
| 24.10 | 支持 | ucode |

## 许可证

本项目采用 GPL v3 许可证。

## 贡献

欢迎提交 issue 和 pull request 来改进这个项目。

## 联系方式

- 项目主页：https://github.com/yourusername/acctl
- 问题反馈：https://github.com/yourusername/acctl/issues
