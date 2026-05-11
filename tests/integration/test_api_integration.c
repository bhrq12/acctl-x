/*
 * ============================================================================
 *
 *       Filename:  test_api_integration.c
 *
 *    Description:  集成测试 - API 调用流程
 *                  测试 API 端点与后端函数的集成
 *
 *        Version:  1.0
 *        Created:  2026-05-11
 *
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

/* 测试配置 */
#define TEST_API_HOST "127.0.0.1"
#define TEST_API_PORT 8080
#define TEST_TIMEOUT_SEC 5

/* 全局变量 */
static volatile int g_test_server_pid = 0;
static volatile int g_test_passed = 0;
static volatile int g_test_failed = 0;

/* 信号处理 */
static void signal_handler(int sig)
{
    (void)sig;
    if (g_test_server_pid > 0) {
        kill(g_test_server_pid, SIGTERM);
    }
    exit(1);
}

/* ============================================================================
 * HTTP 客户端辅助函数
 * ============================================================================ */

typedef struct {
    int status_code;
    char *body;
    size_t body_len;
} http_response_t;

static int http_send_request(const char *method, const char *path,
                            const char *body, http_response_t *resp)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TEST_API_PORT);
    inet_pton(AF_INET, TEST_API_HOST, &addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock);
        return -1;
    }

    /* 构建 HTTP 请求 */
    char request[4096];
    int len = snprintf(request, sizeof(request),
        "%s %s HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Content-Type: application/json\r\n",
        method, path, TEST_API_HOST, TEST_API_PORT);

    if (body && strlen(body) > 0) {
        len += snprintf(request + len, sizeof(request) - len,
            "Content-Length: %zu\r\n", strlen(body));
    }

    len += snprintf(request + len, sizeof(request) - len, "\r\n");

    if (body && strlen(body) > 0) {
        len += snprintf(request + len, sizeof(request) - len, "%s", body);
    }

    /* 发送请求 */
    if (send(sock, request, len, 0) < 0) {
        perror("send");
        close(sock);
        return -1;
    }

    /* 接收响应 */
    char response[8192];
    int received = recv(sock, response, sizeof(response) - 1, 0);
    close(sock);

    if (received < 0) {
        perror("recv");
        return -1;
    }

    response[received] = '\0';

    /* 解析状态码 */
    char *body_start = strstr(response, "\r\n\r\n");
    if (body_start) {
        body_start += 4;
        resp->body = strdup(body_start);
        resp->body_len = strlen(resp->body);
    } else {
        resp->body = strdup("");
        resp->body_len = 0;
    }

    /* 提取状态码 */
    if (sscanf(response, "HTTP/1.1 %d", &resp->status_code) != 1) {
        resp->status_code = 0;
    }

    return 0;
}

/* ============================================================================
 * 测试用例
 * ============================================================================ */

static void test_api_get_aps(void)
{
    printf("[TEST] GET /api/aps - 获取AP列表\n");

    http_response_t resp = {0};
    int ret = http_send_request("GET", "/api/aps", NULL, &resp);

    if (ret == 0 && resp.status_code == 200) {
        printf("  ✓ 状态码: %d\n", resp.status_code);
        printf("  ✓ 响应体: %s\n", resp.body);
        g_test_passed++;
    } else {
        printf("  ✗ 失败: ret=%d, status=%d\n", ret, resp.status_code);
        g_test_failed++;
    }

    free(resp.body);
}

static void test_api_get_ap_by_mac(void)
{
    printf("[TEST] GET /api/ap/{mac} - 获取单个AP详情\n");

    http_response_t resp = {0};
    int ret = http_send_request("GET", "/api/ap/00:11:22:33:44:55", NULL, &resp);

    if (ret == 0 && resp.status_code == 200) {
        printf("  ✓ 状态码: %d\n", resp.status_code);
        printf("  ✓ 响应体: %s\n", resp.body);
        g_test_passed++;
    } else {
        printf("  ✗ 失败: ret=%d, status=%d\n", ret, resp.status_code);
        g_test_failed++;
    }

    free(resp.body);
}

static void test_api_reboot_ap(void)
{
    printf("[TEST] POST /api/ap/reboot - 重启AP\n");

    char body[256];
    snprintf(body, sizeof(body),
        "{\"mac\":\"00:11:22:33:44:55\",\"action\":\"reboot\"}");

    http_response_t resp = {0};
    int ret = http_send_request("POST", "/api/ap/action", body, &resp);

    if (ret == 0 && resp.status_code == 200) {
        printf("  ✓ 状态码: %d\n", resp.status_code);
        printf("  ✓ 响应体: %s\n", resp.body);
        g_test_passed++;
    } else {
        printf("  ✗ 失败: ret=%d, status=%d\n", ret, resp.status_code);
        g_test_failed++;
    }

    free(resp.body);
}

static void test_api_update_resource(void)
{
    printf("[TEST] PUT /api/resource - 更新资源池\n");

    char body[256];
    snprintf(body, sizeof(body),
        "{\"ip_start\":\"192.168.1.100\",\"ip_end\":\"192.168.1.200\",\"ip_mask\":\"255.255.255.0\"}");

    http_response_t resp = {0};
    int ret = http_send_request("PUT", "/api/resource", body, &resp);

    if (ret == 0 && resp.status_code == 200) {
        printf("  ✓ 状态码: %d\n", resp.status_code);
        printf("  ✓ 响应体: %s\n", resp.body);
        g_test_passed++;
    } else {
        printf("  ✗ 失败: ret=%d, status=%d\n", ret, resp.status_code);
        g_test_failed++;
    }

    free(resp.body);
}

static void test_api_invalid_resource_key(void)
{
    printf("[TEST] PUT /api/resource (invalid key) - 测试无效key\n");

    char body[256];
    snprintf(body, sizeof(body),
        "{\"invalid_key\":\"some_value\"}");

    http_response_t resp = {0};
    int ret = http_send_request("PUT", "/api/resource", body, &resp);

    if (ret == 0 && resp.status_code == 400) {
        printf("  ✓ 正确拒绝无效key: %d\n", resp.status_code);
        g_test_passed++;
    } else {
        printf("  ✗ 应该返回400，实际: ret=%d, status=%d\n", ret, resp.status_code);
        g_test_failed++;
    }

    free(resp.body);
}

/* ============================================================================
 * 主函数
 * ============================================================================ */

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("========================================\n");
    printf("  acctl 集成测试 - API 调用流程\n");
    printf("========================================\n\n");

    /* 注意：这些测试需要 API 服务器运行在 %s:%d\n", TEST_API_HOST, TEST_API_PORT); */
    printf("注意: 请确保 acctl-ac 服务运行在 %s:%d\n\n", TEST_API_HOST, TEST_API_PORT);

    /* 运行测试用例 */
    test_api_get_aps();
    test_api_get_ap_by_mac();
    test_api_reboot_ap();
    test_api_update_resource();
    test_api_invalid_resource_key();

    printf("\n========================================\n");
    printf("  测试结果: %d 通过, %d 失败\n",
           g_test_passed, g_test_failed);
    printf("========================================\n");

    return g_test_failed > 0 ? 1 : 0;
}
