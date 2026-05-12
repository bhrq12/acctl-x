# AC Controller (Python Implementation)

基于Python的OpenWrt AC控制器软件包，用于集中管理和控制AP设备集群。

## 功能特性

### 核心功能
- **设备发现** - 自动发现网络中的AP设备
- **配置管理** - 集中管理AP设备配置
- **状态监控** - 实时监控AP运行状态
- **固件升级** - 支持AP固件远程升级
- **流量统计** - 收集和展示AP流量数据
- **安全策略** - CHAP认证、API Token管理

### 技术特性
- 基于标准库http.server实现RESTful API（兼容Python 3.6+）
- SQLite数据库持久化
- CAPWAP协议支持
- 完整的CLI管理工具
- 符合OpenWrt打包规范

## 兼容性

### Python版本支持
| 版本 | 状态 | 说明 |
|------|------|------|
| Python 3.6 | ✅ 完全兼容 | 所有功能支持 |
| Python 3.7 | ✅ 完全兼容 | 所有功能支持 |
| Python 3.8 | ✅ 完全兼容 | 所有功能支持 |
| Python 3.9+ | ✅ 完全兼容 | 所有功能支持 |

### OpenWrt版本支持
| 版本 | Python版本 | 状态 |
|------|-----------|------|
| OpenWrt 19.07 | 3.7 | ✅ 兼容 |
| OpenWrt 21.02 | 3.9 | ✅ 兼容 |
| OpenWrt 22.03 | 3.10 | ✅ 兼容 |
| OpenWrt 23.05 | 3.11 | ✅ 兼容 |
| OpenWrt 24.11 | 3.11+ | ✅ 兼容 |
| ImmortalWrt | 3.10+ | ✅ 兼容 |

## 项目结构

```
acctl-py/
├── Makefile              # OpenWrt软件包Makefile
├── README.md             # 项目说明文档
├── files/                # 安装文件
│   ├── etc/
│   │   ├── config/       # UCI配置文件
│   │   └── init.d/       # procd服务脚本
│   └── usr/bin/          # CLI入口脚本
├── src/                  # Python源代码
│   ├── acctl/            # 主模块
│   │   ├── api/          # REST API模块（标准库实现）
│   │   │   ├── __init__.py
│   │   │   └── main.py   # HTTP服务器实现
│   │   ├── capwap/       # CAPWAP协议模块
│   │   ├── utils/        # 工具函数
│   │   ├── ap_manager.py # AP管理器
│   │   ├── config.py     # 配置管理
│   │   ├── db.py         # 数据库操作
│   │   ├── logger.py     # 日志系统
│   │   ├── security.py   # 安全模块
│   │   └── ac_server.py  # AC服务主程序
│   └── cli.py            # CLI入口模块
└── tests/                # 测试目录
    └── unit/             # 单元测试
```

## 安装与部署

### 依赖要求

```
python3-light
python3-sqlite3
python3-logging
python3-json
python3-threading
python3-multiprocessing
python3-ssl
```

### OpenWrt打包

```bash
# 在OpenWrt构建环境中
make package/acctl-py/compile V=s
make package/acctl-py/install
```

### 服务管理

```bash
# 启用服务
/etc/init.d/acctl-py enable

# 启动服务
/etc/init.d/acctl-py start

# 停止服务
/etc/init.d/acctl-py stop

# 重启服务
/etc/init.d/acctl-py restart
```

## API接口

### 基础信息

- **地址**: `http://<ac-ip>:8080`
- **文档**: `http://<ac-ip>:8080/docs`
- **认证**: API Token

### 主要接口

| 方法 | 路径 | 描述 |
|------|------|------|
| GET | `/api/v1/aps` | 获取所有AP列表 |
| GET | `/api/v1/aps/{mac}` | 获取单个AP详情 |
| POST | `/api/v1/aps` | 注册新AP |
| PUT | `/api/v1/aps/{mac}` | 更新AP配置 |
| DELETE | `/api/v1/aps/{mac}` | 删除AP |
| GET | `/api/v1/stats` | 获取统计信息 |
| GET | `/api/v1/health` | 健康检查 |
| POST | `/api/v1/token` | 获取API Token |

### 获取Token

```bash
curl -X POST "http://localhost:8080/api/v1/token" -H "Content-Type: application/json" -d '{"username": "admin"}'
```

### 示例响应

```json
{
    "success": true,
    "data": {
        "token": "abc123def456...",
        "expires_in": 86400,
        "username": "admin"
    },
    "message": "Token generated successfully",
    "timestamp": 1704067200
}
```

## CLI命令

```bash
# 查看AP列表
acctl-py-cli ap list

# 查看AP详情
acctl-py-cli ap show 00:11:22:33:44:55

# 更新AP配置
acctl-py-cli ap config 00:11:22:33:44:55 --name "Office AP"

# 查看AC状态
acctl-py-cli status

# 查看日志
acctl-py-cli logs --limit 50
```

## 配置文件

配置文件位于 `/etc/config/acctl-py`：

```
config acctl-py 'global'
    option enabled '1'
    option uuid 'auto'
    option name 'OpenWrt-AC-Py'

config acctl-py 'network'
    option listen_port '8080'
    option capwap_port '5246'
    list nic 'br-lan'

config acctl-py 'resource'
    option ip_start '192.168.1.200'
    option ip_end '192.168.1.254'
    option ip_mask '255.255.255.0'

config acctl-py 'security'
    option password ''
    option dtls_enabled '0'

config acctl-py 'logging'
    option level 'info'
    option syslog '1'
```

## 开发

### 运行测试

```bash
cd acctl-py
python -m pytest tests/unit/ -v
```

### 本地开发

```bash
# 启动API服务器
cd src
python -m acctl.api.main

# 启动AC服务器
python -m acctl.ac_server
```

## 许可证

Apache-2.0 License

## 作者

jianxi sun <ycsunjane@gmail.com>