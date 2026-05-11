# acctl 函数追踪表

## 概述
本文档维护项目中所有公共函数的声明和实现位置，用于代码审查和追踪。

## 命名规范
- 函数名：下划线分隔小写 (`db_ap_list`)
- 变量名：下划线分隔小写 (`json_buf`)
- 常量名：全大写下划线分隔 (`MSG_AC_REBOOT`)
- 类型名：下划线分隔小写 (`db_t`)

## 函数清单

### AC 模块 (src/ac/)

#### 数据库模块 (db.c/db.h)

| 函数名 | 声明位置 | 实现位置 | 功能 | 状态 |
|--------|---------|---------|------|------|
| `db_init` | db.h:74 | db.c | 初始化数据库 | ✅ |
| `db_close` | db.h:75 | db.c | 关闭数据库 | ✅ |
| `db_save` | db.h:76 | db.c | 保存数据库 | ✅ |
| `db_query_res` | db.h:84 | db.c | 查询资源 | ✅ |
| `db_update_resource` | db.h:85 | db.c:508 | 更新资源 | ✅ |
| `db_ap_list` | db.h:92 | db.c:540 | 列出所有AP | ✅ |
| `db_ap_get` | db.h:95 | db.c:558 | 获取AP详情 | ✅ |
| `db_ap_upsert` | db.h:100 | db.c | 插入/更新AP | ✅ |
| `db_ap_update_field` | db.h:103 | db.c | 更新AP字段 | ✅ |
| `db_ap_get_field` | db.h:106 | db.c:768 | 获取AP字段 | ✅ |
| `db_ap_set_offline` | db.h:109 | db.c | 标记AP离线 | ✅ |
| `db_group_create` | db.h:115 | db.c | 创建分组 | ✅ |
| `db_group_delete` | db.h:116 | db.c | 删除分组 | ✅ |
| `db_group_list` | db.h:117 | db.c | 列出分组 | ✅ |
| `db_group_add_ap` | db.h:118 | db.c | AP加入分组 | ✅ |
| `db_group_remove_ap` | db.h:119 | db.c | AP移出分组 | ✅ |
| `db_alarm_insert` | db.h:124 | db.c | 插入告警 | ✅ |
| `db_alarm_ack` | db.h:127 | db.c | 确认告警 | ✅ |
| `db_alarm_list` | db.h:128 | db.c | 列出告警 | ✅ |
| `db_alarm_count_by_level` | db.h:129 | db.c | 告警统计 | ✅ |
| `db_firmware_insert` | db.h:134 | db.c | 插入固件 | ✅ |
| `db_firmware_list` | db.h:137 | db.c | 列出固件 | ✅ |
| `db_firmware_delete` | db.h:138 | db.c | 删除固件 | ✅ |
| `db_firmware_getlatest` | db.h:139 | db.c | 获取最新固件 | ✅ |

#### HTTP 服务器模块 (http.c/http.h)

| 函数名 | 声明位置 | 实现位置 | 功能 | 状态 |
|--------|---------|---------|------|------|
| `http_server_init` | http.h:69 | http.c | 初始化HTTP服务器 | ✅ |
| `http_server_start` | http.h:70 | http.c | 启动HTTP服务器 | ✅ |
| `http_server_stop` | http.h:71 | http.c | 停止HTTP服务器 | ✅ |
| `api_init` | http.h:74 | http.c | 初始化API | ✅ |
| `api_register_route` | http.h:75 | http.c | 注册API路由 | ✅ |
| `http_parse_request` | http.h:79 | http.c | 解析请求 | ✅ |
| `http_build_response` | http.h:80 | http.c | 构建响应 | ✅ |
| `http_response_json` | http.h:81 | http.c | JSON响应 | ✅ |
| `http_response_error` | http.h:82 | http.c | 错误响应 | ✅ |
| `http_request_get_header` | http.h:83 | http.c:39 | 获取请求头 | ✅ |

#### 消息处理模块 (process.c/process.h)

| 函数名 | 声明位置 | 实现位置 | 功能 | 状态 |
|--------|---------|---------|------|------|
| `ac_init` | process.h:41 | process.c | 初始化AC | ✅ |
| `ap_lost` | process.h:42 | process.c:436 | AP断开 | ✅ |
| `msg_proc` | process.h:43 | process.c:493 | 消息处理 | ✅ |
| `ap_send_reboot` | process.h:44 | process.c:490 | 发送重启命令 | ✅ |
| `ac_message_insert` | process.h:47 | message.c | 插入消息队列 | ✅ |
| `message_travel_init` | process.h:48 | message.c | 初始化消息队列 | ✅ |

#### 网络模块 (net.c/net.h)

| 函数名 | 声明位置 | 实现位置 | 功能 | 状态 |
|--------|---------|---------|------|------|
| `net_init` | net.h:36 | net.c | 初始化网络 | ✅ |
| `net_send` | net.h:47 | net.c | 发送数据 | ✅ |
| `net_send_tcp` | net.h:57 | net.c | TCP发送 | ✅ |

### AP 模块 (src/ap/)

| 函数名 | 声明位置 | 实现位置 | 功能 | 状态 |
|--------|---------|---------|------|------|
| `init_report` | process.h:73 | ap/process.c | 初始化上报 | ✅ |
| `ap_msg_proc` | process.h:74 | ap/process.c | AP消息处理 | ✅ |
| `__net_netrcv` | process.h:75 | ap/net.c | 网络接收线程 | ✅ |
| `get_uptime` | process.h:77 | ap/apstatus.c | 获取运行时间 | ✅ |
| `get_memfree` | process.h:78 | ap/apstatus.c | 获取空闲内存 | ✅ |
| `get_cpu_usage` | process.h:79 | ap/apstatus.c | 获取CPU使用率 | ✅ |

### CAPWAP 协议模块 (src/lib/capwap/)

| 函数名 | 声明位置 | 实现位置 | 功能 | 状态 |
|--------|---------|---------|------|------|
| `capwap_send_discovery` | capwap.h:91 | capwap.c:340 | 发送发现请求 | ✅ |
| `capwap_send_join` | capwap.h:92 | capwap.c:369 | 发送加入请求 | ✅ |
| `capwap_send_configure` | capwap.h:93 | capwap.c:399 | 发送配置请求 | ✅ |
| `capwap_send_echo` | capwap.h:94 | capwap.c:431 | 发送心跳 | ✅ |
| `capwap_send_reset` | capwap.h:95 | capwap.c:459 | 发送重置请求 | ✅ |
| `capwap_send_statistics` | capwap.h:96 | capwap.c:487 | 发送统计请求 | ✅ |
| `capwap_send_event` | capwap.h:97 | capwap.c:515 | 发送事件 | ✅ |
| `capwap_build_discovery_request` | capwap_msg.h:83 | capwap_msg.c:46 | 构建发现请求 | ✅ |
| `capwap_build_join_request` | capwap_msg.h:86 | capwap_msg.c:94 | 构建加入请求 | ✅ |
| `capwap_build_echo_request` | capwap_msg.h:89 | capwap_msg.c:142 | 构建心跳请求 | ✅ |
| `capwap_build_wtp_event` | capwap_msg.h:92 | capwap_msg.c:162 | 构建WTP事件 | ✅ |
| `capwap_build_configure_request` | capwap_msg.h:95 | capwap_msg.c:199 | 构建配置请求 | ✅ |
| `capwap_build_reset_request` | capwap_msg.h:98 | capwap_msg.c:243 | 构建重置请求 | ✅ |
| `capwap_build_statistics_request` | capwap_msg.h:101 | capwap_msg.c:263 | 构建统计请求 | ✅ |
| `capwap_parse_header` | capwap_msg.h:104 | capwap_msg.c | 解析头部 | ✅ |
| `capwap_parse_message` | capwap_msg.h:110 | capwap_msg.c | 解析消息 | ✅ |
| `capwap_free_parsed_message` | capwap_msg.h:113 | capwap_msg.c | 释放消息 | ✅ |
| `msg_map_to_capwap` | capwap_msg.h:119 | capwap_msg.c:443 | 消息映射到CAPWAP | ✅ |
| `msg_map_from_capwap` | capwap_msg.h:120 | capwap_msg.c | CAPWAP映射到消息 | ✅ |
| `capwap_image_transfer_init` | capwap_msg.h:131 | capwap_msg.c:619 | 镜像传输初始化 | ✅ |

### 参数处理模块 (src/lib/)

| 函数名 | 声明位置 | 实现位置 | 功能 | 状态 |
|--------|---------|---------|------|------|
| `help` | arg.h:55 | arg.c | 显示帮助 | ✅ |
| `proc_cfgarg` | arg.h:56 | cfgarg.c:100 | 处理配置参数 | ✅ |
| `proc_cmdarg` | arg.h:57 | cmdarg.c:146 | 处理命令参数 | ✅ |
| `proc_arg` | arg.h:58 | arg.c:120 | 处理参数 | ✅ |

### 安全模块 (src/lib/sec.c)

| 函数名 | 功能 | 状态 |
|--------|------|------|
| `sec_get_password` | 获取密码 | ✅ |
| `sec_set_password` | 设置密码 | ✅ |
| `sec_encrypt` | 加密数据 | ✅ |
| `sec_decrypt` | 解密数据 | ✅ |
| `sec_verify_hmac` | 验证HMAC | ✅ |

## 更新记录

| 日期 | 版本 | 更新内容 |
|------|------|---------|
| 2026-05-11 | 1.0 | 初始版本，包含所有公共函数 |

## 使用说明

### 添加新函数
1. 在相应的 `.c` 文件中实现函数
2. 在相应的 `.h` 文件中添加声明
3. 更新本文档，添加函数记录

### 代码审查检查清单
- [ ] 所有调用的函数是否已在头文件中声明
- [ ] 所有声明的函数是否已在源文件中实现
- [ ] 函数返回值是否符合调用者的预期
- [ ] 参数类型是否匹配
