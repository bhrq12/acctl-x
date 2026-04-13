#!/bin/bash
# OpenWrt Build Script
# 用法: ./scripts/build.sh
# 依赖环境变量: PLATFORMS, ARTIFACT

set -euo pipefail

# ========== 配置区 ==========
OPENWRT_REPO="https://github.com/coolsnowwolf/lede"
OPENWRT_BRANCH="master"
ACCTL_REPO="https://github.com/你的用户名/acctl"
ACCTL_BRANCH="main"
BUILD_THREADS=4

# ========== 颜色输出 ==========
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info()  { echo -e "${GREEN}[INFO]${NC}  $1"; }
log_warn()  { echo -e "${YELLOW}[WARN]${NC}  $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# ========== 初始化 ==========
log_info "初始化编译环境..."

# 创建工作目录
WORKDIR="$GITHUB_WORKSPACE/openwrt-build"
mkdir -p "$WORKDIR"
cd "$WORKDIR"

# 加载 feeds 配置（如果存在）
if [ -f "$GITHUB_WORKSPACE/configs/feeds.conf" ]; then
    log_info "检测到自定义 feeds.conf"
    FEEDS_CONF="$GITHUB_WORKSPACE/configs/feeds.conf"
else
    FEEDS_CONF=""
fi

# ========== 平台配置映射 ==========
declare -A PLATFORM_CONFIG
PLATFORM_CONFIG[ipq40xx]="target/linux/ipq40xx"
PLATFORM_CONFIG[x86_64]="target/linux/x86"
PLATFORM_CONFIG[bcm2711]="target/linux/bcm2711"
PLATFORM_CONFIG[ramips-mt7621]="target/linux/ramips/mt7621"

# ========== 单平台编译函数 ==========
build_platform() {
    local platform=$1
    log_info "========== 开始编译平台: $platform =========="

    local platform_dir="$WORKDIR/build-$platform"
    local config_src="$GITHUB_WORKSPACE/configs/${platform}.config"
    local output_firmware="$WORKDIR/output/firmware"
    local output_packages="$WORKDIR/output/packages"

    mkdir -p "$platform_dir" "$output_firmware" "$output_packages"

    cd "$platform_dir"

    # 1. 克隆源码（缓存复用）
    if [ ! -d ".git" ]; then
        log_info "克隆 OpenWrt 源码..."
        git clone --depth=1 -b "$OPENWRT_BRANCH" "$OPENWRT_REPO" .
    fi

    # 2. 添加 acctl 软件包 feeds
    log_info "添加 acctl 软件包..."
    if [ -d "package/acctl" ]; then
        log_warn "acctl 已存在，跳过添加"
    else
        # 方式A: 从独立仓库引入
        git clone --depth=1 -b "$ACCTL_BRANCH" "$ACCTL_REPO" "package/acctl"
    fi

    # 3. 加载自定义 feeds.conf
    if [ -n "$FEEDS_CONF" ]; then
        log_info "应用自定义 feeds 配置..."
        cat "$FEEDS_CONF" >> feeds.conf.default
        ./scripts/feeds update -a
        ./scripts/feeds install -a
    fi

    # 4. 应用平台配置
    if [ -f "$config_src" ]; then
        log_info "应用平台配置: $config_src"
        cp "$config_src" .config
        # 增量编译保护：非必要不清理
        log_info "执行 make defconfig..."
        make defconfig
    else
        log_error "找不到平台配置: configs/${platform}.config"
        log_error "请先在 configs/ 目录创建 ${platform}.config"
        return 1
    fi

    # 5. 编译固件
    if [[ "$ARTIFACT" == "firmware" || "$ARTIFACT" == "both" ]]; then
        log_info "编译固件..."
        make -j"$BUILD_THREADS" V=s
        # 收集固件产物
        find bin/targets -name "*.bin" -o -name "*.img" 2>/dev/null \
            | xargs -I{} cp {} "$output_firmware/"
        log_info "固件已输出至: $output_firmware"
    fi

    # 6. 编译软件包
    if [[ "$ARTIFACT" == "packages" || "$ARTIFACT" == "both" ]]; then
        log_info "编译软件包..."
        make package/acctl/compile V=s -j"$BUILD_THREADS"
        make package/index V=s
        # 收集 ipk 产物
        find bin/packages -name "*.ipk" 2>/dev/null \
            | xargs -I{} cp {} "$output_packages/"
        log_info "软件包已输出至: $output_packages"
    fi

    log_info "========== 平台 $platform 编译完成 =========="
}

# ========== 主流程 ==========
log_info "PLATFORMS=$PLATFORMS"
log_info "ARTIFACT=$ARTIFACT"

for platform in $PLATFORMS; do
    if [ -z "${PLATFORM_CONFIG[$platform]:-}" ]; then
        log_error "不支持的平台: $platform"
        continue
    fi

    if ! build_platform "$platform"; then
        log_error "平台 $platform 编译失败，终止"
        exit 1
    fi
done

log_info "全部编译完成！"
ls -lh "$WORKDIR/output/firmware" 2>/dev/null || true
ls -lh "$WORKDIR/output/packages" 2>/dev/null || true
