# AC控制器Python版本开发方案

## 1. 项目概述

### 1.1 项目背景

本项目旨在基于OpenWrt平台，开发一个纯Python实现的AC（接入控制器）软件包，用于集中管理和控制运行`acctl-ap`软件包的AP（接入点）设备集群。该软件包将作为现有C语言实现`acctl-ac`的替代方案，提供更灵活的开发和维护体验。

### 1.2 功能目标

参照`acctl-ac`软件包，实现以下核心功能：

| 功能模块 | 功能描述 |
|---------|---------|
| 设备发现 | 自动发现并注册网络中的AP设备 |
| 配置管理 | 集中管理AP设备的配置参数 |
| 状态监控 | 实时监控AP设备运行状态 |
| 固件升级 | 支持AP设备固件远程升级 |
| 流量统计 | 收集和展示AP流量数据 |
| 安全策略 | 设备认证、DTLS加密、访问控制 |

### 1.3 技术选型

| 分类 | 技术 | 版本要求 | 选型理由 |
|------|------|---------|---------|
| 语言 | Python | 3.8+ | OpenWrt 24.11默认版本，兼容性好 |
| Web框架 | FastAPI | 0.100+ | 高性能异步框架，自动API文档 |
| 数据库 | SQLite | 内置 | 轻量级，零依赖，适合嵌入式环境 |
| CAPWAP协议 | 自定义实现 | - | 参照acctl-ac的CAPWAP实现 |
| 进程管理 | procd | - | OpenWrt标准进程管理框架 |

---

## 2. 技术架构设计

### 2.1 整体架构

```
┌─────────────────────────────────────────────────────────────────┐
│                    外部接口层                                    │
│  ┌─────────┐  ┌──────────┐  ┌───────────┐  ┌──────────────┐   │
│  │  LuCI   │  │ REST API │  │ CAPWAP    │  │ CLI 命令行   │   │
│  │  Web UI │  │ 接口     │  │ 协议接口  │  │ 管理工具     │   │
│  └────┬────┘  └────┬─────┘  └─────┬─────┘  └──────┬───────┘   │
└───────┼────────────┼──────────────┼────────────────┼───────────┘
        │            │              │                │
┌───────▼────────────▼──────────────▼────────────────▼───────────┐
│                    业务逻辑层                                    │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐    │
│  │ AP管理器    │  │ 配置管理器  │  │ 消息处理器          │    │
│  │ (APManager) │  │ (ConfigMgr) │  │ (MessageProcessor)  │    │
│  └──────┬──────┘  └──────┬──────┘  └──────────┬──────────┘    │
│         │                │                     │                │
│         └────────┬───────┴─────────────────────┘                │
│                  ▼                                             │
│  ┌───────────────────────────────────────────────┐              │
│  │              核心服务层                        │              │
│  │  ┌────────┐  ┌──────────┐  ┌─────────────┐   │              │
│  │  │ 认证   │  │ 安全策略 │  │ 日志系统    │   │              │
│  │  │ 模块   │  │  引擎    │  │  (Logger)   │   │              │
│  │  └────────┘  └──────────┘  └─────────────┘   │              │
│  └───────────────────────────────────────────────┘              │
└─────────────────────────────────────────────────────────────────┘
        │
┌───────▼─────────────────────────────────────────────────────────┐
│                    数据持久层                                    │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │              SQLite数据库                                │    │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐│    │
│  │  │ ap_info  │  │ ap_config│  │ ac_config│  │  logs    ││    │
│  │  └──────────┘  └──────────┘  └──────────┘  └──────────┘│    │
│  └─────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────┘
```

### 2.2 模块划分

| 模块 | 文件名 | 职责描述 |
|------|--------|---------|
| **ac_server** | `ac_server.py` | AC服务主进程，协调整体功能 |
| **ap_manager** | `ap_manager.py` | AP设备管理（发现、注册、状态维护） |
| **capwap** | `capwap/` | CAPWAP协议实现（消息解析、DTLS加密） |
| **config** | `config.py` | 配置管理（UCI配置解析、运行时配置） |
| **api** | `api/` | REST API实现（FastAPI路由） |
| **db** | `db.py` | 数据库操作封装 |
| **security** | `security.py` | 安全认证（CHAP、API Token） |
| **logger** | `logger.py` | 日志记录系统 |
| **cli** | `cli.py` | 命令行管理工具 |

### 2.3 目录结构

```
acctl-py/
├── Makefile                      # OpenWrt软件包Makefile
├── files/                        # 安装文件
│   ├── etc/
│   │   ├── config/
│   │   │   └── acctl-py          # UCI配置文件
│   │   └── init.d/
│   │       └── acctl-py          # procd服务脚本
│   └── usr/
│       └── bin/
│           └── acctl-py-cli      # CLI入口脚本
├── src/                          # Python源代码
│   ├── acctl/
│   │   ├── __init__.py
│   │   ├── ac_server.py          # AC服务主程序
│   │   ├── ap_manager.py         # AP设备管理器
│   │   ├── config.py             # 配置管理
│   │   ├── db.py                 # 数据库操作
│   │   ├── security.py           # 安全模块
│   │   ├── logger.py             # 日志模块
│   │   ├── capwap/               # CAPWAP协议模块
│   │   │   ├── __init__.py
│   │   │   ├── protocol.py       # CAPWAP协议实现
│   │   │   ├── dtls.py           # DTLS加密
│   │   │   └── messages.py       # 消息定义
│   │   ├── api/                  # REST API模块
│   │   │   ├── __init__.py
│   │   │   ├── main.py           # FastAPI应用入口
│   │   │   ├── routes/           # API路由
│   │   │   │   ├── __init__.py
│   │   │   │   ├── ap.py         # AP管理API
│   │   │   │   ├── config.py     # 配置API
│   │   │   │   └── stats.py      # 统计API
│   │   │   └── schemas/          # Pydantic模型
│   │   │       ├── __init__.py
│   │   │       ├── ap.py
│   │   │       └── config.py
│   │   └── utils/                # 工具函数
│   │       ├── __init__.py
│   │       └── network.py        # 网络工具
│   └── cli.py                    # CLI入口模块
├── tests/                        # 测试目录
│   ├── unit/                     # 单元测试
│   └── integration/              # 集成测试
└── README.md                     # 项目说明文档
```

---

## 3. 功能模块详细设计

### 3.1 AP设备管理模块

**功能说明**：负责AP设备的发现、注册、状态监控和管理。

**核心类：APManager**

| 方法名 | 功能描述 | 参数 | 返回值 |
|--------|---------|------|--------|
| `discover_aps()` | 发现网络中的AP设备 | 无 | `List[APInfo]` |
| `register_ap(mac, info)` | 注册新AP设备 | `mac`: str, `info`: dict | `bool` |
| `deregister_ap(mac)` | 注销AP设备 | `mac`: str | `bool` |
| `get_ap_status(mac)` | 获取AP状态 | `mac`: str | `APStatus` |
| `get_all_aps()` | 获取所有AP列表 | 无 | `List[APInfo]` |
| `update_ap_config(mac, config)` | 更新AP配置 | `mac`: str, `config`: dict | `bool` |

**数据结构：APInfo**

| 字段名 | 类型 | 说明 |
|--------|------|------|
| `mac` | str | AP设备MAC地址 |
| `name` | str | AP设备名称 |
| `model` | str | 设备型号 |
| `firmware_version` | str | 固件版本 |
| `ip_address` | str | IP地址 |
| `status` | str | 状态（online/offline） |
| `uptime` | int | 运行时间（秒） |
| `clients` | int | 连接客户端数 |
| `last_seen` | datetime | 最后活跃时间 |

### 3.2 CAPWAP协议模块

**功能说明**：实现CAPWAP协议的消息解析、封装和传输。

**核心类：CAPWAPProtocol**

| 方法名 | 功能描述 | 参数 | 返回值 |
|--------|---------|------|--------|
| `send_discover_response()` | 发送发现响应 | `socket`, `addr`, `ac_info` | None |
| `handle_join_request()` | 处理加入请求 | `msg`: CAPWAPMessage | `CAPWAPMessage` |
| `handle_configuration_request()` | 处理配置请求 | `msg`: CAPWAPMessage | `CAPWAPMessage` |
| `send_configuration_update()` | 发送配置更新 | `ap_mac`, `config` | None |
| `send_firmware_request()` | 发送固件升级请求 | `ap_mac`, `firmware_path` | None |

**CAPWAP消息类型**：

| 消息类型 | 代码 | 说明 |
|---------|------|------|
| DISCOVER_REQUEST | 1 | AP发现请求 |
| DISCOVER_RESPONSE | 2 | AC发现响应 |
| JOIN_REQUEST | 3 | AP加入请求 |
| JOIN_RESPONSE | 4 | AC加入响应 |
| CONFIGURATION_REQUEST | 5 | AP配置请求 |
| CONFIGURATION_RESPONSE | 6 | AC配置响应 |
| CAPWAP_CONTROL_MESSAGE | 7 | 控制消息 |

### 3.3 配置管理模块

**功能说明**：管理AC和AP的配置参数，支持UCI配置文件解析。

**核心类：ConfigManager**

| 方法名 | 功能描述 | 参数 | 返回值 |
|--------|---------|------|--------|
| `load_config()` | 加载UCI配置 | 无 | `Config` |
| `save_config(config)` | 保存配置 | `config`: dict | `bool` |
| `get_ac_config()` | 获取AC全局配置 | 无 | `dict` |
| `get_ap_config(mac)` | 获取指定AP配置 | `mac`: str | `dict` |
| `set_ap_config(mac, config)` | 设置AP配置 | `mac`: str, `config`: dict | `bool` |

**配置结构**：

```python
{
    "global": {
        "enabled": True,
        "uuid": "auto",
        "name": "OpenWrt-AC"
    },
    "network": {
        "listen_port": 8080,
        "capwap_port": 5246,
        "nic": ["br-lan"]
    },
    "resource": {
        "ip_start": "192.168.1.200",
        "ip_end": "192.168.1.254",
        "ip_mask": "255.255.255.0"
    },
    "security": {
        "password": "",
        "dtls_enabled": False
    },
    "logging": {
        "level": "info",
        "syslog": True
    }
}
```

### 3.4 REST API模块

**功能说明**：提供RESTful API接口，支持外部系统集成。

**API路由设计**：

| HTTP方法 | 路径 | 功能描述 | 认证要求 |
|---------|------|---------|---------|
| GET | `/api/v1/aps` | 获取所有AP列表 | 是 |
| GET | `/api/v1/aps/{mac}` | 获取单个AP详情 | 是 |
| POST | `/api/v1/aps` | 注册新AP | 是 |
| PUT | `/api/v1/aps/{mac}` | 更新AP配置 | 是 |
| DELETE | `/api/v1/aps/{mac}` | 删除AP | 是 |
| GET | `/api/v1/aps/{mac}/status` | 获取AP状态 | 是 |
| POST | `/api/v1/aps/{mac}/upgrade` | 升级AP固件 | 是 |
| GET | `/api/v1/config` | 获取AC配置 | 是 |
| PUT | `/api/v1/config` | 更新AC配置 | 是 |
| GET | `/api/v1/stats` | 获取统计数据 | 是 |
| POST | `/api/v1/token` | 获取API Token | 是（密码认证） |

### 3.5 安全模块

**功能说明**：提供认证、授权和加密功能。

**核心类：SecurityManager**

| 方法名 | 功能描述 | 参数 | 返回值 |
|--------|---------|------|--------|
| `generate_token(username)` | 生成API Token | `username`: str | `str` |
| `validate_token(token)` | 验证Token有效性 | `token`: str | `bool` |
| `chap_authenticate(challenge, response, password)` | CHAP认证 | 见参数说明 | `bool` |
| `generate_challenge()` | 生成CHAP挑战 | 无 | `str` |
| `hash_password(password)` | 密码哈希 | `password`: str | `str` |

**CHAP认证流程**：

```
AC                         AP
│                           │
│  1. 发送 Challenge       │
│  ────────────────────────►│
│                           │
│  2. 接收 Response        │
│  ◄───────────────────────│
│                           │
│  3. 验证并发送结果        │
│  ────────────────────────►│
```

### 3.6 日志模块

**功能说明**：提供统一的日志记录功能。

**核心类：Logger**

| 方法名 | 功能描述 | 参数 | 返回值 |
|--------|---------|------|--------|
| `debug(msg, **kwargs)` | 调试日志 | `msg`: str | None |
| `info(msg, **kwargs)` | 信息日志 | `msg`: str | None |
| `warn(msg, **kwargs)` | 警告日志 | `msg`: str | None |
| `error(msg, **kwargs)` | 错误日志 | `msg`: str | None |
| `critical(msg, **kwargs)` | 严重错误日志 | `msg`: str | None |

**日志格式**：
```
[2024-01-01 12:00:00] [INFO] [ap_manager] AP 00:11:22:33:44:55 registered
```

---

## 4. 接口规范

### 4.1 外部接口

#### 4.1.1 CAPWAP协议接口

| 接口类型 | 端口 | 协议 | 说明 |
|---------|------|------|------|
| CAPWAP控制通道 | 5246 | UDP | AP与AC通信主通道 |
| CAPWAP数据通道 | 5247 | UDP | 数据转发通道 |
| REST API | 8080 | TCP/HTTP | 管理接口 |

#### 4.1.2 API接口规范

**响应格式**：
```json
{
    "success": true,
    "data": { ... },
    "message": "操作成功",
    "timestamp": 1704067200
}
```

**错误响应格式**：
```json
{
    "success": false,
    "error": {
        "code": 404,
        "message": "AP not found"
    },
    "timestamp": 1704067200
}
```

#### 4.1.3 CLI接口

```bash
# 查看AP列表
acctl-py-cli ap list

# 查看单个AP信息
acctl-py-cli ap show <mac>

# 更新AP配置
acctl-py-cli ap config <mac> --name <name>

# 重启AP
acctl-py-cli ap restart <mac>

# 固件升级
acctl-py-cli ap upgrade <mac> --firmware <path>

# 查看AC状态
acctl-py-cli status

# 重新加载配置
acctl-py-cli reload
```

### 4.2 内部接口

#### 4.2.1 模块间调用关系

| 调用方 | 被调用方 | 调用场景 |
|--------|---------|---------|
| `ac_server` | `ap_manager` | 初始化AP管理 |
| `ac_server` | `capwap.protocol` | 启动CAPWAP服务 |
| `ac_server` | `api.main` | 启动API服务 |
| `ap_manager` | `db` | 持久化AP数据 |
| `ap_manager` | `security` | 认证AP设备 |
| `api.routes` | `ap_manager` | API请求处理 |
| `capwap.protocol` | `security` | DTLS加密 |

---

## 5. 开发计划

### 5.1 里程碑规划

| 阶段 | 时间 | 目标 | 交付物 |
|------|------|------|--------|
| **Phase 1** | 第1-2周 | 基础架构搭建 | 项目结构、配置管理、日志模块 |
| **Phase 2** | 第3-4周 | 核心功能开发 | CAPWAP协议、AP发现注册 |
| **Phase 3** | 第5-6周 | 管理功能完善 | 配置管理、状态监控 |
| **Phase 4** | 第7-8周 | API与CLI开发 | REST API、CLI工具 |
| **Phase 5** | 第9-10周 | 安全与优化 | CHAP认证、DTLS、性能优化 |
| **Phase 6** | 第11-12周 | 测试与集成 | 单元测试、集成测试、文档 |

### 5.2 详细任务清单

**Phase 1: 基础架构搭建**

| 任务 | 描述 | 负责人 | 估计时间 |
|------|------|--------|---------|
| P1-01 | 创建项目目录结构 | 架构师 | 1天 |
| P1-02 | 编写Makefile模板 | 开发工程师 | 2天 |
| P1-03 | 实现配置管理模块 | 开发工程师 | 3天 |
| P1-04 | 实现日志模块 | 开发工程师 | 2天 |
| P1-05 | 实现数据库模块 | 开发工程师 | 3天 |

**Phase 2: 核心功能开发**

| 任务 | 描述 | 负责人 | 估计时间 |
|------|------|--------|---------|
| P2-01 | CAPWAP协议消息定义 | 开发工程师 | 3天 |
| P2-02 | CAPWAP消息解析器 | 开发工程师 | 4天 |
| P2-03 | AP发现机制实现 | 开发工程师 | 3天 |
| P2-04 | AP注册/注销流程 | 开发工程师 | 3天 |

**Phase 3: 管理功能完善**

| 任务 | 描述 | 负责人 | 估计时间 |
|------|------|--------|---------|
| P3-01 | AP配置管理 | 开发工程师 | 3天 |
| P3-02 | 状态监控模块 | 开发工程师 | 4天 |
| P3-03 | 固件升级功能 | 开发工程师 | 4天 |
| P3-04 | 流量统计模块 | 开发工程师 | 3天 |

**Phase 4: API与CLI开发**

| 任务 | 描述 | 负责人 | 估计时间 |
|------|------|--------|---------|
| P4-01 | FastAPI应用搭建 | 开发工程师 | 2天 |
| P4-02 | AP管理API | 开发工程师 | 3天 |
| P4-03 | 配置管理API | 开发工程师 | 2天 |
| P4-04 | CLI工具开发 | 开发工程师 | 3天 |

**Phase 5: 安全与优化**

| 任务 | 描述 | 负责人 | 估计时间 |
|------|------|--------|---------|
| P5-01 | CHAP认证实现 | 开发工程师 | 3天 |
| P5-02 | DTLS加密支持 | 开发工程师 | 4天 |
| P5-03 | API Token认证 | 开发工程师 | 2天 |
| P5-04 | 性能优化 | 开发工程师 | 3天 |

**Phase 6: 测试与集成**

| 任务 | 描述 | 负责人 | 估计时间 |
|------|------|--------|---------|
| P6-01 | 单元测试编写 | 测试工程师 | 4天 |
| P6-02 | 集成测试 | 测试工程师 | 3天 |
| P6-03 | OpenWrt打包测试 | 开发工程师 | 2天 |
| P6-04 | 文档编写 | 技术文档工程师 | 3天 |

---

## 6. 测试策略

### 6.1 测试分层

| 测试层次 | 测试目标 | 测试方法 | 工具 |
|---------|---------|---------|------|
| **单元测试** | 验证单个函数/方法 | 隔离测试 | pytest |
| **集成测试** | 验证模块间交互 | 端到端测试 | pytest + requests |
| **系统测试** | 验证整体功能 | 黑盒测试 | curl/httpie |
| **性能测试** | 验证性能指标 | 压力测试 | locust |

### 6.2 测试覆盖范围

**单元测试覆盖**：

| 模块 | 测试用例 | 覆盖要点 |
|------|---------|---------|
| `config.py` | 配置加载/保存 | UCI解析、配置验证 |
| `db.py` | CRUD操作 | 数据持久化、查询效率 |
| `security.py` | 认证功能 | Token生成、CHAP认证 |
| `capwap/protocol.py` | 消息解析 | 协议编解码、错误处理 |

**集成测试覆盖**：

| 测试场景 | 测试内容 | 预期结果 |
|---------|---------|---------|
| AP发现 | 模拟AP发送发现请求 | AC正确响应 |
| AP注册 | 模拟AP注册流程 | AP成功注册到AC |
| 配置下发 | 修改AP配置 | 配置正确下发到AP |
| 固件升级 | 触发固件升级 | AP成功升级固件 |
| API认证 | 无Token访问API | 返回401错误 |

### 6.3 性能测试指标

| 指标 | 目标值 | 说明 |
|------|-------|------|
| AP接入能力 | ≥100个 | 支持100+ AP并发接入 |
| API响应时间 | ≤100ms | 单次API请求响应时间 |
| 内存占用 | ≤50MB | 运行时内存消耗 |
| CPU占用 | ≤20% | 正常运行时CPU使用率 |

---

## 7. 部署与集成

### 7.1 OpenWrt打包规范

**Makefile结构**：

```makefile
include $(TOPDIR)/rules.mk

PKG_NAME:=acctl-py
PKG_VERSION:=1.0
PKG_RELEASE:=1
PKG_MAINTAINER:=jianxi sun <ycsunjane@gmail.com>
PKG_DESCRIPTION:=AC Controller - Python Implementation

include $(INCLUDE_DIR)/package.mk
include $(TOPDIR)/feeds/packages/lang/python/python3-package.mk

define Package/acctl-py
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=Access Controllers
  TITLE:=AC Controller (Python)
  DEPENDS:=+python3-light +python3-fastapi +python3-uvicorn +python3-sqlite3 +python3-cryptography
  URL:=https://github.com/bhrq12/acctl
endef

# ... 其他定义
```

**依赖说明**：

| 依赖包 | 用途 | 版本要求 |
|--------|------|---------|
| python3-light | Python运行时 | 3.8+ |
| python3-fastapi | Web框架 | 0.100+ |
| python3-uvicorn | ASGI服务器 | 0.20+ |
| python3-sqlite3 | 数据库 | 内置 |
| python3-cryptography | 加密库 | 40+ |

### 7.2 服务管理

**procd脚本结构**：

```bash
#!/bin/sh /etc/rc.common

START=99
STOP=10

USE_PROCD=1

start_service() {
    procd_open_instance
    procd_set_param command /usr/bin/python3 /usr/lib/acctl-py/ac_server.py
    procd_set_param user root
    procd_set_param stdout 1
    procd_set_param stderr 1
    procd_set_param respawn 3 5
    procd_close_instance
}

stop_service() {
    killall -TERM ac_server.py
}
```

### 7.3 日志配置

**日志输出路径**：
- 应用日志：`/var/log/acctl-py/ac_server.log`
- 系统日志：通过syslog记录到`/var/log/messages`

---

## 8. 安全最佳实践

### 8.1 安全设计原则

| 原则 | 实现措施 |
|------|---------|
| **最小权限** | 服务以非root用户运行（可选） |
| **输入验证** | 所有用户输入进行严格验证和过滤 |
| **数据加密** | DTLS加密CAPWAP通信 |
| **认证授权** | CHAP认证AP设备，API Token认证 |
| **日志审计** | 完整记录所有操作和异常 |

### 8.2 安全检查清单

- [ ] 所有外部输入进行验证（长度、格式、内容）
- [ ] 避免SQL注入（使用参数化查询）
- [ ] 避免命令注入（不使用shell=True）
- [ ] API接口使用Token认证
- [ ] 敏感信息不记录到日志
- [ ] 配置文件权限设置为600
- [ ] 使用HTTPS（生产环境）

---

## 9. 代码规范

### 9.1 PEP 8规范

- 使用4个空格缩进
- 变量/函数使用蛇形命名法（snake_case）
- 类使用大驼峰命名法（CamelCase）
- 常量使用全大写下划线分隔
- 行长度不超过79字符
- 导入顺序：标准库 → 第三方库 → 自定义库

### 9.2 错误处理

- 禁止使用空捕获异常（bare except）
- 明确指定捕获的异常类型
- 提供有意义的错误信息
- 使用logging记录异常而非print

### 9.3 文档规范

- 所有模块、类、函数必须有docstring
- docstring格式遵循Google风格
- 复杂逻辑添加必要注释
- 提供API文档（FastAPI自动生成）

---

## 10. 交付物清单

| 交付物 | 描述 | 状态 |
|--------|------|------|
| 源代码 | 完整Python实现 | 待开发 |
| Makefile | OpenWrt软件包Makefile | 待开发 |
| 配置文件 | UCI配置模板 | 待开发 |
| 服务脚本 | procd初始化脚本 | 待开发 |
| CLI工具 | 命令行管理工具 | 待开发 |
| 单元测试 | pytest测试用例 | 待开发 |
| 集成测试 | 端到端测试用例 | 待开发 |
| API文档 | Swagger UI文档 | 自动生成 |
| README | 项目说明文档 | 待编写 |

---

**文档版本**: v1.0  
**创建日期**: 2026-05-12  
**作者**: jianxi sun  
**状态**: 待评审