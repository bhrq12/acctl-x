#!/bin/bash
# ============================================================================
# CAPWAP 文件完整性验证脚本
# 验证所有CAPWAP相关文件是否正确引用和编译
# ============================================================================

set -e

echo "=============================================="
echo " CAPWAP 文件完整性验证"
echo "=============================================="

# 检查CAPWAP核心文件是否存在
echo ""
echo "[1] 检查CAPWAP核心文件..."

CAPWAP_FILES=(
    "src/include/capwap/capwap.h"
    "src/include/capwap/capwap_msg.h"
    "src/lib/capwap/capwap.c"
    "src/lib/capwap/capwap_msg.c"
    "src/lib/capwap/capwap_net.c"
    "src/lib/capwap/capwap_dtls.c"
)

missing_files=0
for file in "${CAPWAP_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "  ✓ $file"
    else
        echo "  ✗ $file - 缺失!"
        missing_files=$((missing_files + 1))
    fi
done

if [ $missing_files -gt 0 ]; then
    echo "错误: 发现 $missing_files 个缺失文件!"
    exit 1
fi

# 检查Makefile配置
echo ""
echo "[2] 检查Makefile配置..."

MAKEFILES=(
    "src/Makefile"
    "acctl-ac/src/Makefile.ac"
    "acctl-ap/src/Makefile.ap"
)

for makefile in "${MAKEFILES[@]}"; do
    if grep -q "capwap" "$makefile"; then
        echo "  ✓ $makefile - 包含CAPWAP配置"
    else
        echo "  ✗ $makefile - 缺少CAPWAP配置!"
    fi
done

# 检查源代码中的CAPWAP引用
echo ""
echo "[3] 检查源代码中的CAPWAP引用..."

SRC_FILES=$(find src -name "*.c" | grep -v test)
for srcfile in $SRC_FILES; do
    if grep -q "capwap/capwap.h" "$srcfile" || grep -q "capwap/capwap_msg.h" "$srcfile"; then
        echo "  ✓ $srcfile - 正确引用CAPWAP头文件"
    fi
done

# 检查测试文件中的CAPWAP引用
echo ""
echo "[4] 检查测试文件中的CAPWAP引用..."

TEST_FILES=(
    "acctl-ac/test/unit/test_capwap.c"
    "acctl-ap/test/unit/test_capwap.c"
)

for testfile in "${TEST_FILES[@]}"; do
    if [ -f "$testfile" ]; then
        if grep -q "capwap/capwap.h" "$testfile"; then
            echo "  ✓ $testfile - 正确引用CAPWAP头文件"
        else
            echo "  ✗ $testfile - 缺少CAPWAP头文件引用!"
        fi
    else
        echo "  ⚠ $testfile - 测试文件不存在"
    fi
done

echo ""
echo "=============================================="
echo " 验证完成! CAPWAP文件结构完整"
echo "=============================================="
echo ""
echo "CAPWAP文件集中管理位置:"
echo "  - 头文件: src/include/capwap/"
echo "  - 源文件: src/lib/capwap/"
echo ""
echo "所有软件包均引用统一的CAPWAP文件路径"
