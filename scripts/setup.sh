#!/bin/bash
# 环境初始化脚本（预留，未来可扩展）
# 目前编译环境在 GitHub Actions 容器中已预装必要工具
# 此脚本可按需扩展：安装额外依赖、配置 git、设置 ssh key 等

set -euo pipefail

echo "[setup] 环境初始化..."

# 检查必要工具
for cmd in git make gcc g++ python3; do
    if command -v "$cmd" &>/dev/null; then
        echo "[setup] ✓ $cmd"
    else
        echo "[setup] ✗ $cmd 未安装"
        exit 1
    fi
done

echo "[setup] 环境就绪。"
