# OpenWrt 云编译配置仓库

> 纯配置驱动，不存放源码。按需选择平台和产物，手动触发编译。

## 功能特性

- 🔧 **多平台支持**：ipq40xx / x86_64 / bcm2711 / ramips-mt7621 / 全平台
- 📦 **产物可选**：完整固件（.bin）/ 软件包（.ipk）/ 两者都编译
- 🔒 **手动触发**：通过 GitHub Actions `workflow_dispatch` 安全可控
- 📁 **配置分离**：每个平台独立 `.config`，方便按设备定制

## 平台说明

| 平台 | 架构 | 适用设备 |
|------|------|----------|
| `ipq40xx` | ARM Cortex-A7 | 小米 CR660x/AX1800/AX3600、TP-Link XDR/XE 系列等 |
| `x86_64` | x86_64 | 任意 x86_64 设备、软路由 |
| `bcm2711` | ARM Cortex-A72 | 树莓派 4B |
| `ramips-mt7621` | MIPS | 红米 AC2100、京东云系列等 |

## 使用方法

### 1. Fork 本仓库

### 2. 修改 acctl 软件包地址

编辑 `scripts/build.sh`，修改以下两行：

```bash
ACCTL_REPO="https://github.com/你的用户名/acctl"
ACCTL_BRANCH="main"
```

### 3. 添加平台配置

每个平台需要一个 `.config` 文件，来源两种方式：

**方式A：从现有设备迁移（推荐）**
```bash
# 在本地 OpenWrt 编译环境中
make menuconfig  # 选择平台 + 软件包
./scripts/diffconfig > configs/ipq40xx.config
```

**方式B：复制现有 OpenWrt lean 的配置**
```bash
# 找到编译好的 .config 文件
cp lede/.config configs/ipq40xx.config
```

### 4. 触发编译

1. 进入本仓库 → **Actions** → **OpenWrt Build**
2. 点击 **Run workflow**
3. 选择平台和产物类型
4. 点击 **Run workflow**

### 5. 下载产物

编译完成后在对应 workflow run 页面下载：
- 固件：`openwrt-firmware-<平台>.zip`
- 软件包：`openwrt-packages-<平台>.zip`

## 目录结构

```
.
├── .github/workflows/
│   └── build.yml        # GitHub Actions 工作流（手动触发）
├── configs/
│   ├── feeds.conf       # 自定义 feeds 配置
│   ├── ipq40xx.config   # ipq40xx 平台配置
│   ├── x86_64.config    # x86_64 平台配置
│   └── ...
├── scripts/
│   ├── build.sh         # 编译脚本
│   └── setup.sh         # 环境初始化（预留）
└── README.md
```

## 添加新平台

1. 在 `configs/` 创建 `<平台>.config`
2. 在 `scripts/build.sh` 的 `PLATFORM_CONFIG` 数组添加映射
3. 在 `.github/workflows/build.yml` 的 `platform` 选项添加新平台
4. 提交即可

## CI 环境

- Ubuntu 22.04
- 编译线程数：4（可调整 `scripts/build.sh` 中的 `BUILD_THREADS`）
- 超时时间：360 分钟
- 产物保留：14 天

## 常见问题

**Q: 编译失败怎么办？**  
A: 点击对应 workflow run → 查看 build log，定位报错信息。常见问题：.config 缺少必要依赖、 feeds 失效等。

**Q: 如何加速编译？**  
A: 在 `scripts/build.sh` 中将 `BUILD_THREADS` 调大，或使用 `make -j$(nproc)` 使用全部核心。

**Q: acctl 软件包如何更新？**  
A: 每次编译时会重新 clone 最新代码。如需指定版本，修改 `ACCTL_BRANCH` 为对应 tag 或 commit hash。
