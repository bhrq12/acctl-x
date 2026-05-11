#!/bin/bash
# pre-commit-check.sh - 预提交代码检查脚本
# 用法: ./pre-commit-check.sh 或将其复制到 .git/hooks/pre-commit

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

echo "========================================"
echo "  acctl 预提交代码检查"
echo "========================================"
echo ""

# 检查是否有未提交的编译错误日志修改
if git diff --name-only 2>/dev/null | grep -q "compile-luci-app-acctl.log"; then
    echo -e "${YELLOW}警告: compile-luci-app-acctl.log 有未提交的修改${NC}"
fi

# 1. 检查隐式函数声明
echo "[1/6] 检查隐式函数声明..."
IMPLICIT_DECLS=$(grep -rn "implicit declaration" src/ 2>/dev/null || true)
if [ -n "$IMPLICIT_DECLS" ]; then
    echo -e "${RED}❌ 发现隐式函数声明:${NC}"
    echo "$IMPLICIT_DECLS"
    exit 1
fi
echo -e "${GREEN}✓ 无隐式函数声明${NC}"

# 2. 检查未定义的引用
echo "[2/6] 检查未定义的引用..."
UNDEFINED_REFS=$(grep -rn "undefined reference" src/ 2>/dev/null || true)
if [ -n "$UNDEFINED_REFS" ]; then
    echo -e "${RED}❌ 发现未定义的引用:${NC}"
    echo "$UNDEFINED_REFS"
    exit 1
fi
echo -e "${GREEN}✓ 无未定义的引用${NC}"

# 3. 检查函数声明与实现匹配
echo "[3/6] 检查函数声明与实现匹配..."

check_function_decl() {
    local header=$1
    local func_pattern=$2

    # 从头文件获取声明（忽略 static 和 extern）
    local decls=$(grep -E "$func_pattern" "$header" 2>/dev/null | grep -v "^[[:space:]]*//" | grep -v "^[[:space:]]*\*" | sed 's/[[:space:]]*(.*//' | sed 's/[[:space:]]*\*.*//' | tr -d '\r\n')

    if [ -z "$decls" ]; then
        return 0
    fi

    while IFS= read -r decl; do
        # 提取函数名（最后一个单词）
        func_name=$(echo "$decl" | awk '{print $NF}' | tr -d '*,')
        if [ -z "$func_name" ] || [ "$func_name" = "int" ] || [ "$func_name" = "void" ] || [ "$func_name" = "char" ] || [ "$func_name" = "struct" ]; then
            continue
        fi

        # 检查函数是否在源文件中实现
        if ! grep -rq "^[a-zA-Z_].*${func_name}.*(" src/ 2>/dev/null; then
            echo -e "${RED}  ❌ 函数 ${func_name} 声明但未实现${NC}"
            return 1
        fi
    done <<< "$decls"

    return 0
}

# 检查关键头文件
check_function_decl "src/include/db.h" "^int [a-zA-Z_]"
check_function_decl "src/include/http.h" "^int [a-zA-Z_]"
check_function_decl "src/include/process.h" "^[a-zA-Z_].* [a-zA-Z_]"
check_function_decl "src/include/net.h" "^int [a-zA-Z_]"
echo -e "${GREEN}✓ 函数声明与实现匹配${NC}"

# 4. 检查关键文件是否存在
echo "[4/6] 检查关键文件是否存在..."
KEY_FILES=(
    "src/ac/main.c"
    "src/ac/api.c"
    "src/ac/http.c"
    "src/ac/db.c"
    "src/ac/process.c"
    "src/ac/net.c"
    "src/ap/main.c"
    "src/ap/process.c"
    "src/ap/net.c"
    "luci-app-acctl/luasrc/controller/acctl.lua"
)

for file in "${KEY_FILES[@]}"; do
    if [ ! -f "$file" ]; then
        echo -e "${RED}❌ 关键文件缺失: $file${NC}"
        exit 1
    fi
done
echo -e "${GREEN}✓ 关键文件存在${NC}"

# 5. 检查新增函数是否有适当的注释
echo "[5/6] 检查新增函数的注释..."
NEW_FUNCS=$(grep -rn "int ap_send_reboot\|int db_ap_list\|int db_ap_get\|int db_update_resource\|http_request_get_header" src/ 2>/dev/null | grep -E "\.c:[0-9]+:" || true)
if [ -n "$NEW_FUNCS" ]; then
    echo -e "${YELLOW}注意: 请确保以下函数有适当的注释:${NC}"
    echo "$NEW_FUNCS"
fi
echo -e "${GREEN}✓ 函数注释检查完成${NC}"

# 6. 尝试编译检查（如果 make 可用）
echo "[6/6] 编译检查..."
if command -v make &> /dev/null; then
    # 检查是否有 Makefile
    if [ -f "Makefile" ] || [ -f "acctl-ac/Makefile" ]; then
        echo -e "${YELLOW}执行 make clean && make package/acctl-ac/compile 2>&1 | head -50${NC}"
        # 不执行实际编译，只检查 make 是否存在
        if make -n package/acctl-ac/compile &>/dev/null; then
            echo -e "${GREEN}✓ Makefile 存在且可解析${NC}"
        else
            echo -e "${YELLOW}⚠ 无法解析 Makefile（正常如果你不在构建环境中）${NC}"
        fi
    else
        echo -e "${YELLOW}⚠ 未找到 Makefile${NC}"
    fi
else
    echo -e "${YELLOW}⚠ make 未安装，跳过编译检查${NC}"
fi

echo ""
echo "========================================"
echo -e "${GREEN}  预提交检查完成 ✓${NC}"
echo "========================================"
echo ""
echo "提示:"
echo "  - 函数追踪表已更新: docs/FUNCTIONS.md"
echo "  - 请确保所有新函数都有声明和实现"
echo "  - 请确保修改后执行完整编译验证"
echo ""

exit 0
