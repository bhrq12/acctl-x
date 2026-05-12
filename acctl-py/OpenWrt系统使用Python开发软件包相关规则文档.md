# OpenWrt系统使用Python开发软件包相关规则文档

# 第一章 总则

## 1\.1 文档目的

为规范OpenWrt系统下Python开发软件包的流程，规避依赖冲突、系统崩溃、卸载残留、LuCI报错等问题，保障软件包的兼容性、稳定性和可维护性，统一开发标准，特制定本规则。本规则适用于所有基于OpenWrt系统（24\.11\+版本，apk包管理器）、采用Python3开发的自定义软件包，是开发、打包、测试、部署全流程的核心依据。

## 1\.2 适用范围

本规则适用于所有开发OpenWrt Python软件包的开发人员，涵盖软件包从需求设计、编码开发、打包编译、测试验证到部署卸载的全生命周期，包括但不限于工具类、控制类（如AC控制器）、服务类Python软件包。

## 1\.3 核心原则

- 兼容性优先：不依赖系统核心库，适配OpenWrt官方Python环境，避免版本冲突和系统破坏；

- 轻量高效：优先使用轻量依赖和简洁编码，适配路由器嵌入式设备的资源限制；

- 安全可控：禁止危险操作，确保安装、卸载、运行过程不影响OpenWrt系统正常功能；

- 规范统一：遵循OpenWrt官方打包标准和Python编码规范，保证软件包结构、接口、文档的一致性。

# 第二章 开发环境规范

## 2\.1 开发环境要求

- OpenWrt源码环境：需搭建对应版本的OpenWrt源码编译环境，确保源码完整，feeds配置正确（需包含官方packages feed，参考OpenWrt Python包开发规范）；

- Python版本：统一使用Python3，优先适配OpenWrt官方提供的python3\-light（轻量版，占用空间小，适合嵌入式设备），禁止使用Python2（已停止维护，OpenWrt已移除所有Python2包）；

- 依赖管理：依赖包优先选择OpenWrt官方软件源中的包，禁止使用未经过验证的第三方源，避免引入恶意代码或不兼容依赖。

## 2\.2 环境配置规范

在OpenWrt源码根目录下，确保feeds配置正确，包含Python相关包的feed源，示例配置（feeds\.conf\.default）：

```bash
src-git packages https://git.openwrt.org/feed/packages.git
src-git luci https://git.openwrt.org/project/luci.git
src-git routing https://git.openwrt.org/feed/routing.git
src-git telephony https://git.openwrt.org/feed/telephony.git
```

配置完成后，执行以下命令更新feeds并安装Python相关依赖：

```bash
./scripts/feeds update -a
./scripts/feeds install -a
```

# 第三章 软件包结构规范

## 3\.1 标准目录结构

Python软件包需遵循OpenWrt官方包目录结构，统一放在OpenWrt源码的package/目录下，标准结构如下（以软件包名称acctl为例）：

```bash
package/acctl/                  # 软件包根目录（名称与软件包名一致，小写）
├── Makefile                   # 核心编译配置文件（必选）
├── preinst                    # 安装前校验脚本（必选，禁止--force安装）
├── postinst                   # 安装后配置脚本（必选，启动服务、刷新LuCI）
├── postrm                     # 卸载后清理脚本（必选，无残留删除）
└── files/                     # 业务文件目录（必选，存放Python脚本、服务文件等）
    ├── etc/
    │   └── init.d/
    │       └── acctl          # 开机自启服务脚本（可选，服务类软件包必选）
    └── usr/
        └── bin/
            └── acctl.py       # Python核心业务脚本（必选）
```

## 3\.2 目录及文件命名规范

- 软件包根目录名称：统一使用小写字母，无特殊字符、空格，与Makefile中PKG\_NAME一致；

- Python脚本：后缀为\.py，命名简洁明了，与功能相关（如acctl\.py、gpio\_control\.py），禁止使用中文、特殊字符；

- 服务脚本：与软件包名称一致，无后缀（如acctl），放在files/etc/init\.d/目录下；

- 安装/卸载脚本：固定命名为preinst、postinst、postrm，无后缀，具有可执行权限（chmod \+x）。

## 3\.3 文件权限规范

- Python脚本：权限设置为755（rwxr\-xr\-x），确保可执行；

- 服务脚本：权限设置为755，符合OpenWrt init脚本权限标准；

- 安装/卸载脚本：权限设置为755，确保OpenWrt包管理器能正常执行；

- 所有文件所有者、所属组统一为root:root，避免权限异常导致脚本无法执行。

# 第四章 编码规范

## 4\.1 基础编码规范

- 编码格式：统一使用UTF\-8编码，避免中文乱码；

- 解释器声明：所有Python脚本开头必须添加解释器声明，指定OpenWrt系统中的Python路径：\#\!/usr/bin/python3；

- 代码风格：遵循PEP 8编码规范，缩进使用4个空格，变量命名采用小写字母\+下划线（snake\_case），函数命名与变量命名规范一致，类命名采用驼峰式（CamelCase）；

- 注释规范：关键代码块（如核心逻辑、参数配置、异常处理）必须添加注释，说明功能、参数含义、返回值，注释简洁明了，不冗余。

## 4\.2 功能编码规范

- 轻量性：避免引入不必要的依赖和冗余代码，优先使用Python标准库，如需第三方依赖，必须选择轻量版本，且在Makefile中明确声明；

- 异常处理：核心逻辑必须添加try\-except异常捕获，避免脚本崩溃导致系统异常，异常信息需清晰，便于排查问题，禁止使用bare except（无具体异常类型的except）；

- 日志输出：脚本运行日志需输出到指定路径（推荐/tmp/目录，如/tmp/acctl\.log），日志包含时间、操作、结果、异常信息，便于问题排查，禁止直接打印到控制台影响系统输出；

- 资源释放：涉及串口、GPIO、网络连接等资源的操作，必须在使用完成后释放资源（如关闭串口、释放GPIO引脚），避免资源泄露；

- 容错设计：脚本需具备基础容错能力，如参数错误、资源不可用时，能正常退出并输出错误信息，不导致系统卡死。

## 4\.3 禁止性编码要求

- 禁止直接操作系统核心文件（如/etc/rc\.common、/lib/ubox等），避免破坏系统运行；

- 禁止使用os\.system执行危险命令（如rm \-rf /、killall python等），如需执行系统命令，需进行严格的参数校验，优先使用subprocess模块替代os\.system；

- 禁止硬编码路径、端口、配置信息，如需配置，优先从OpenWrt系统配置文件（/etc/config/目录下）读取；

- 禁止引入与功能无关的依赖包，避免增加软件包体积和兼容性风险；

- 禁止使用多线程、多进程过度消耗路由器资源，嵌入式设备优先使用单线程，如需多任务，需控制资源占用。

# 第五章 Makefile规范

## 5\.1 Makefile基础配置规范

Makefile是Python软件包编译、打包的核心，必须遵循OpenWrt官方规范，参考OpenWrt Python包Makefile示例，基础配置如下：

```makefile
include $(TOPDIR)/rules.mk

# 软件包基础信息（必选，需根据实际情况修改）
PKG_NAME:=acctl                  # 软件包名称，与目录名称一致
PKG_VERSION:=1.0                 # 版本号（遵循语义化版本，如1.0.0）
PKG_RELEASE:=1                   # 发布版本，每次修改递增
PKG_MAINTAINER:=Your Name <your@mail.com>  # 维护者信息

# 依赖声明（核心，杜绝冲突）
# 只依赖官方Python轻量包和稳定接口包，不依赖系统核心库（如libubox、procd）
PKG_DEPENDS:=+python3-light +luci-base +rpcd-mod-luci

# 编译目录配置（固定）
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
include $(INCLUDE_DIR)/package.mk

# 软件包描述（必选）
define Package/acctl
  SECTION:=utils                  # 软件包分类（如utils、net、lang等）
  CATEGORY:=Utilities             # 分类名称，与SECTION对应
  TITLE:=Python AC Controller for OpenWrt  # 软件包标题
  URL:=https://openwrt.org        # 可选，软件包相关URL
endef

# 软件包详细描述（可选，建议添加）
define Package/acctl/description
 AC控制器（Python版）- 轻量无依赖冲突，适配OpenWrt 24.11+版本，
 支持AC设备控制、状态监测，具备开机自启、异常重启功能。
endef

# 编译步骤（纯Python脚本无需编译，留空即可）
define Build/Compile
endef

# 安装步骤（必选，将files目录下的文件安装到固件对应路径）
define Package/acctl/install
	# 创建安装目录
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/init.d
	
	# 安装Python脚本和服务脚本
	$(INSTALL_BIN) ./files/usr/bin/acctl.py $(1)/usr/bin/
	$(INSTALL_BIN) ./files/etc/init.d/acctl $(1)/etc/init.d/
endef

# 固定编译指令（必选）
$(eval $(call BuildPackage,$(PKG_NAME)))
```

## 5\.2 依赖配置核心规则

- 依赖最小化：只声明软件包运行必需的依赖，禁止声明无关依赖，优先使用python3\-light（轻量版），避免使用python3（完整版，占用空间大）；

- 依赖安全性：依赖包必须是OpenWrt官方源中的包，禁止依赖第三方非官方包，避免依赖冲突和安全风险；

- 禁止依赖系统核心库：严禁依赖base\-files、procd、libubox、libubus等系统核心包，避免因依赖版本不兼容导致系统核心包被卸载；

- 依赖版本：如需指定依赖版本，需使用兼容匹配（如\+python3\-light\&gt;=3\.8），避免指定固定版本导致兼容性问题。

## 5\.3 编译配置规范

- 纯Python脚本无需编写编译逻辑，Build/Compile部分留空即可；

- 安装路径规范：Python脚本统一安装到/usr/bin/目录，服务脚本统一安装到/etc/init\.d/目录，配置文件统一安装到/etc/config/目录；

- 使用OpenWrt官方安装指令：安装文件使用$\(INSTALL\_DIR\)创建目录，$\(INSTALL\_BIN\)安装可执行文件，禁止使用cp命令直接复制。

# 第六章 安装/卸载脚本规范

## 6\.1 preinst（安装前校验脚本）

核心作用：禁止使用\-\-force安装，从源头规避系统核心包被卸载的风险，脚本内容规范如下：

```bash
#!/bin/sh
# 禁止使用--force、-f等强制安装参数，检测到则退出安装
if echo "$@" | grep -qE 'force|--force|-f'; then
    echo "❌ 错误：禁止使用 --force 安装 $PKG_NAME！会破坏系统核心组件"
    exit 1
fi
# 校验Python环境是否存在
if [ ! -x "/usr/bin/python3" ]; then
    echo "❌ 错误：系统未安装Python3环境，无法安装 $PKG_NAME"
    exit 1
fi
exit 0
```

## 6\.2 postinst（安装后配置脚本）

核心作用：配置软件包运行环境、启动服务、刷新LuCI（如有LuCI界面），脚本内容规范如下：

```bash
#!/bin/sh
# 启用并启动服务（服务类软件包必选）
/etc/init.d/acctl enable
/etc/init.d/acctl start 2>/dev/null

# 刷新LuCI缓存（如有LuCI界面）
/etc/init.d/uhttpd restart 2>/dev/null
/etc/init.d/rpcd restart 2>/dev/null

# 输出安装成功信息
echo "✅ $PKG_NAME 安装完成，服务已启动，可通过LuCI后台或命令行管理"
exit 0
```

## 6\.3 postrm（卸载后清理脚本）

核心作用：彻底删除软件包所有相关文件，避免残留文件导致LuCI报错、系统异常，脚本内容规范如下：

```bash
#!/bin/sh
# 停止并禁用服务
/etc/init.d/acctl stop 2>/dev/null
/etc/init.d/acctl disable 2>/dev/null

# 彻底删除所有相关文件（根据实际文件路径修改）
rm -f /usr/bin/acctl.py
rm -f /etc/init.d/acctl
rm -f /tmp/acctl.log  # 删除日志文件
rm -rf /etc/config/acctl  # 如有配置文件，一并删除

# 刷新LuCI缓存
/etc/init.d/uhttpd restart 2>/dev/null

# 输出卸载成功信息
echo "✅ $PKG_NAME 已完全卸载，无残留文件"
exit 0
```

## 6\.4 脚本通用规则

- 脚本开头必须添加\#\!/bin/sh，确保可被系统shell执行；

- 所有命令需添加错误捕获（如2\&gt;/dev/null），避免脚本执行失败导致安装/卸载中断；

- 脚本执行完成后必须exit 0，确保OpenWrt包管理器识别安装/卸载成功；

- 禁止在脚本中执行危险命令（如rm \-rf /、killall python等），如需删除文件，需指定具体路径，避免误删。

# 第七章 服务管理规范

## 7\.1 服务脚本编写规范

服务类软件包（如AC控制器）需编写OpenWrt标准init服务脚本，基于procd管理，支持开机自启、启停、重启，规范示例如下：

```bash
#!/bin/sh /etc/rc.common

# 服务启动优先级（99为最低，避免与系统核心服务冲突）
START=99
# 服务停止优先级（10为最高，优先停止）
STOP=10
# 启用procd管理（必选，支持崩溃自动重启）
USE_PROCD=1

# 启动服务逻辑
start_service() {
    # 打开procd实例
    procd_open_instance acctl
    # 指定Python脚本启动命令（路径必须正确）
    procd_set_param command /usr/bin/python3 /usr/bin/acctl.py
    # 配置崩溃自动重启（3次失败后，10秒内重试）
    procd_set_param respawn 3 10
    # 日志输出到syslog（可通过logread查看）
    procd_set_param stdout 1
    procd_set_param stderr 1
    # 关闭procd实例
    procd_close_instance
}

# 停止服务逻辑（procd自动处理，可根据需求补充）
stop_service() {
    # 精确杀死当前服务进程，避免误杀其他Python程序
    pids=$(pgrep -f "/usr/bin/python3 /usr/bin/acctl.py")
    if [ -n "$pids" ]; then
        kill $pids 2>/dev/null
        sleep 1
        # 强制杀死未退出的进程
        pids=$(pgrep -f "/usr/bin/python3 /usr/bin/acctl.py")
        if [ -n "$pids" ]; then
            kill -9 $pids 2>/dev/null
        fi
    fi
}

# 重启服务逻辑（可选，默认调用stop_service + start_service）
restart_service() {
    stop_service
    sleep 2
    start_service
}
```

## 7\.2 服务管理规则

- 服务启动优先级（START）：统一设置为99，避免与OpenWrt系统核心服务（如network、uhttpd）冲突；

- 崩溃自动重启：必须配置procd\_set\_param respawn参数，确保服务崩溃后能自动重启，提升稳定性；

- 进程控制：停止服务时，需精确匹配当前服务的进程（使用pgrep \-f），避免误杀其他Python程序；

- 日志管理：服务日志需输出到syslog或指定文件，便于排查服务运行异常；

- 开机自启：在postinst脚本中启用服务（/etc/init\.d/acctl enable），确保系统重启后服务自动启动。

# 第八章 兼容性规范

## 8\.1 固件版本兼容

- 软件包需适配OpenWrt 24\.11\+版本（apk包管理器），禁止适配低于24\.11的版本（opkg包管理器），避免包管理器不兼容；

- 如需适配多个版本，需在Makefile中通过条件判断配置不同依赖，确保在目标版本中正常运行。

## 8\.2 Python版本兼容

- 统一适配Python3\.8\+版本，避免使用Python3\.8以下的特性（如海象运算符:=），确保在OpenWrt官方Python环境中正常运行；

- 禁止使用Python标准库中已废弃的模块和方法，避免版本升级后脚本失效。

## 8\.3 硬件适配规范

- 涉及GPIO、串口、WiFi等硬件操作的软件包，需适配OpenWrt常用路由器硬件（如mt7620、mt7621），避免硬件接口差异导致脚本无法运行；

- 硬件相关操作需添加兼容性判断（如检测GPIO引脚是否存在、串口是否可用），避免硬件不支持导致脚本崩溃。

## 8\.4 依赖版本兼容

- 依赖包优先使用“大于等于”版本匹配（如\+python3\-light\&gt;=3\.8），避免指定固定版本（如\+python3\-light=3\.8\.10），提升兼容性；

- 定期检查依赖包的更新，及时调整依赖配置，避免因依赖包升级导致软件包无法运行。

# 第九章 测试规范

## 9\.1 测试环境要求

- 本地测试：在OpenWrt源码编译环境中，编译软件包，检查编译过程是否无报错，生成的apk包是否完整；

- 路由器测试：使用目标固件（OpenWrt 24\.11\+）的路由器，进行安装、卸载、运行测试，确保在真实环境中正常工作；

- 兼容性测试：在不同硬件型号、不同版本的OpenWrt固件中测试，确保软件包在目标范围内兼容。

## 9\.2 测试内容规范

### 9\.2\.1 安装测试

- 测试正常安装：执行apk add \./acctl\_1\.0\-1\.apk \-\-allow\-untrusted，检查安装是否成功，服务是否自动启动；

- 测试强制安装：执行apk add \-\-force \./acctl\_1\.0\-1\.apk \-\-allow\-untrusted，检查是否能正常拒绝安装；

- 测试依赖缺失：在未安装依赖包（如python3\-light）的环境中安装，检查是否能提示依赖缺失，安装失败。

### 9\.2\.2 运行测试

- 功能测试：验证软件包核心功能是否正常（如AC控制、GPIO操作、网络通信等）；

- 稳定性测试：软件包连续运行24小时以上，检查是否出现崩溃、卡死、资源泄露等问题；

- 异常测试：模拟异常场景（如断网、硬件断开、参数错误），检查脚本是否能正常容错，不导致系统异常。

### 9\.2\.3 卸载测试

- 测试正常卸载：执行apk del acctl，检查卸载是否成功，所有相关文件是否被彻底删除；

- 测试残留检查：卸载后，检查/usr/bin/、/etc/init\.d/、/tmp/等目录，确认无软件包相关残留文件；

- 测试卸载后系统状态：卸载后，检查OpenWrt系统（如LuCI、网络、其他服务）是否正常运行，无异常报错。

## 9\.3 测试报告规范

测试完成后，需生成测试报告，包含以下内容：测试环境（固件版本、硬件型号、Python版本）、测试内容、测试结果、异常情况及解决方案，确保软件包可追溯、可维护。

# 第十章 附则

## 10\.1 规则更新

本规则将根据OpenWrt版本更新、Python版本升级及实际开发需求，定期更新完善，确保规则的适用性和时效性。

## 10\.2 违规处理

开发人员需严格遵循本规则，如因违反规则导致软件包无法运行、系统崩溃、兼容性问题等，需自行承担相应责任，并及时整改。

## 10\.3 参考资料

- OpenWrt官方文档：https://openwrt\.org/docs/guide\-developer/packages

- OpenWrt Python包开发规范：https://github\.com/openwrt/packages/blob/master/lang/python/README\.md

- Python PEP 8编码规范：https://peps\.python\.org/pep\-0008/

- OpenWrt procd服务脚本示例：https://openwrt\.org/docs/guide\-developer/procd\-init\-script\-example

## 10\.4 生效日期

本规则自发布之日起生效。

> （注：文档部分内容可能由 AI 生成）
