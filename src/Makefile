# Makefile for acctl source build
# Supports OpenWrt SDK cross-compilation and standalone builds
#
# Usage (OpenWrt SDK):
#   make -C $(PKG_BUILD_DIR) -f src/Makefile \
#     TOPDIR=$(PKG_BUILD_DIR) \
#     CC="$(TARGET_CC)" \
#     CFLAGS="$(TARGET_CFLAGS) -Wall -Wextra -I$(PKG_BUILD_DIR)/src/include" \
#     acser acctl-cli
#
# Usage (standalone - run from src directory):
#   make -C src

TOPDIR ?= .
INC_DIR := $(TOPDIR)/include
OUT_DIR := .

CC       ?= gcc
CXX      ?= g++
AR       ?= ar
INSTALL  ?= install
RM       ?= rm -f

# Build flags
# For OpenWrt: CFLAGS is passed directly, include path is already in it
# For standalone: use default with local include path
CFLAGS ?= -Wall -Wextra -O2 -g -I$(INC_DIR)
LDFLAGS ?= -lpthread -lm -pthread

# Source directory prefix
# When OpenWrt builds, source is in $(TOPDIR)/src/
# When standalone, source is directly in $(TOPDIR)/
SRC_PREFIX ?=
LIB_DIR := $(SRC_PREFIX)lib
AC_DIR := $(SRC_PREFIX)ac
AP_DIR := $(SRC_PREFIX)ap

# Targets
SERVER ?= 1
CLIENT ?= 1
CLI    ?= 1

.PHONY: all clean acser apctl acctl-cli

all: $(if $(SERVER),acser) $(if $(CLIENT),apctl) $(if $(CLI),acctl-cli)

# ============================================================================
# Shared library sources - located in lib/
# Each target builds its own copy of lib objects in separate directories.
# ============================================================================
LIB_SRCS   = $(LIB_DIR)/chap.c $(LIB_DIR)/md5.c $(LIB_DIR)/link.c $(LIB_DIR)/thread.c $(LIB_DIR)/sec.c $(LIB_DIR)/sha256.c
LIB_SRCS  += $(LIB_DIR)/dllayer.c $(LIB_DIR)/netlayer.c $(LIB_DIR)/arg.c $(LIB_DIR)/cfgarg.c $(LIB_DIR)/cmdarg.c $(LIB_DIR)/mjson.c

# CAPWAP library sources - centralized in lib/capwap/
CAPWAP_SRCS = $(LIB_DIR)/capwap/capwap.c $(LIB_DIR)/capwap/capwap_msg.c $(LIB_DIR)/capwap/capwap_net.c $(LIB_DIR)/capwap/capwap_dtls.c

# Create output directories
$(shell mkdir -p ac_lib ap_lib cli_lib)

# ============================================================================
# AC server (acser) — compiled with -DSERVER
# Build lib objects with -DSERVER in ac_lib/
# ============================================================================
AC_LIB_OBJS = $(patsubst $(LIB_DIR)/%.c,ac_lib/%.o,$(LIB_SRCS))
AC_CAPWAP_OBJS = $(patsubst $(LIB_DIR)/capwap/%.c,ac_lib/capwap_%.o,$(CAPWAP_SRCS))
AC_SRCS     = $(AC_DIR)/main.c $(AC_DIR)/process.c $(AC_DIR)/net.c $(AC_DIR)/message.c \
              $(AC_DIR)/resource.c $(AC_DIR)/db.c $(AC_DIR)/aphash.c $(AC_DIR)/http.c $(AC_DIR)/api.c
AC_OBJS     = $(patsubst $(AC_DIR)/%.c,ac_%.o,$(AC_SRCS))

# AC lib objects with -DSERVER
ac_lib/%.o: $(LIB_DIR)/%.c
	$(CC) $(CFLAGS) -DSERVER -c $< -o $@

# AC CAPWAP objects with -DSERVER
ac_lib/capwap_%.o: $(LIB_DIR)/capwap/%.c
	$(CC) $(CFLAGS) -DSERVER -c $< -o $@

# AC objects with -DSERVER (prefixed with ac_ to avoid conflicts)
ac_%.o: $(AC_DIR)/%.c
	$(CC) $(CFLAGS) -DSERVER -c $< -o $@

acser: $(AC_LIB_OBJS) $(AC_CAPWAP_OBJS) $(AC_OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

# ============================================================================
# AP client (apctl) — compiled with -DCLIENT
# Build lib objects with -DCLIENT in ap_lib/
# ============================================================================
AP_LIB_OBJS = $(patsubst $(LIB_DIR)/%.c,ap_lib/%.o,$(LIB_SRCS))
AP_CAPWAP_OBJS = $(patsubst $(LIB_DIR)/capwap/%.c,ap_lib/capwap_%.o,$(CAPWAP_SRCS))
AP_SRCS     = $(AP_DIR)/main.c $(AP_DIR)/process.c $(AP_DIR)/message.c $(AP_DIR)/net.c $(AP_DIR)/apstatus.c
AP_OBJS     = $(patsubst $(AP_DIR)/%.c,ap_%.o,$(AP_SRCS))

# AP lib objects with -DCLIENT
ap_lib/%.o: $(LIB_DIR)/%.c
	$(CC) $(CFLAGS) -DCLIENT -c $< -o $@

# AP CAPWAP objects with -DCLIENT
ap_lib/capwap_%.o: $(LIB_DIR)/capwap/%.c
	$(CC) $(CFLAGS) -DCLIENT -c $< -o $@

# AP objects with -DCLIENT (prefixed with ap_ to avoid conflicts)
ap_%.o: $(AP_DIR)/%.c
	$(CC) $(CFLAGS) -DCLIENT -c $< -o $@

apctl: $(AP_LIB_OBJS) $(AP_CAPWAP_OBJS) $(AP_OBJS)
	$(CC) $(filter-out -lmicrohttpd,$(LDFLAGS)) $^ -o $@

# ============================================================================
# AC CLI tool (acctl-cli) — no SERVER/CLIENT
# ============================================================================
CLI_LIB_OBJS = $(patsubst $(LIB_DIR)/%.c,cli_lib/%.o,$(LIB_SRCS))
CLI_SRCS     = $(AC_DIR)/cli.c $(AC_DIR)/db.c
CLI_OBJS     = $(patsubst $(AC_DIR)/%.c,cli_%.o,$(CLI_SRCS))

# CLI lib objects without SERVER/CLIENT
cli_lib/%.o: $(LIB_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# CLI objects without SERVER/CLIENT (prefixed with cli_ to avoid conflicts)
cli_%.o: $(AC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

acctl-cli: $(CLI_LIB_OBJS) $(CLI_OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

# ============================================================================
# Clean
# ============================================================================
clean:
	$(RM) -r ac_lib ap_lib cli_lib
	$(RM) ac_*.o ap_*.o cli_*.o acser apctl acctl-cli