# Makefile for acctl shared library and binaries
#
# Builds: libapctl.a (static library with all modules)
# Also builds: acser (AC server), apctl (AP client), acctl-cli (CLI tool)
#
# Supports both standalone compilation and OpenWrt SDK cross-compilation
#
# Usage (standalone):
#   make              # build everything
#   make clean        # clean build artifacts
#
# Usage (OpenWrt SDK):
#   make CC=... CFLAGS=... LDFLAGS=... [SERVER=1|CLIENT=1] [acser|apctl|acctl-cli]
#

# Directory paths
SRC_DIR   = .
INC_DIR   = ../include
OUT_DIR   = .

# Compiler settings
CC       ?= gcc
CFLAGS   ?= -Wall -Wextra -O2 -g
LDFLAGS  ?=

# Default target: build all binaries
TARGETS  = acser apctl acctl-cli

# Source files (shared library)
LIB_SRCS  = chap.c md5.c link.c thread.c sec.c sha256.c
LIB_OBJS  = $(LIB_SRCS:.c=.o)

# AC-specific library sources
AC_LIB_SRCS = dllayer.c netlayer.c arg.c cfgarg.c cmdarg.c mjson.c
AC_LIB_OBJS = $(AC_LIB_SRCS:.c=.o)

# AC server sources
AC_SRCS   = ../ac/main.c ../ac/process.c ../ac/net.c ../ac/message.c \
            ../ac/resource.c ../ac/db.c ../ac/aphash.c ../ac/cli.c
AC_OBJS   = $(AC_SRCS:.c=.o)

# AP client sources
AP_SRCS   = ../ap/main.c ../ap/process.c ../ap/message.c ../ap/net.c \
            ../ap/apstatus.c
AP_OBJS   = $(AP_SRCS:.c=.o)

# Build mode flags
ifdef SERVER
  CFLAGS += -DSERVER -I$(INC_DIR)
  LIB_OBJS := $(AC_LIB_OBJS) $(LIB_OBJS)
  TARGET := acser
else ifdef CLIENT
  CFLAGS += -DCLIENT -I$(INC_DIR)
  LIB_OBJS := $(AC_LIB_OBJS) $(LIB_OBJS)
  TARGET := apctl
endif

# Detect if we're in OpenWrt build environment
ifdef CROSS_COMPILE
  OPENWRT_BUILD := 1
endif

.PHONY: all clean ac_lib ap_lib acser apctl acctl-cli

all: $(TARGETS)
	@echo "Build complete."
	@echo "  acser        -> $(OUT_DIR)/acser"
	@echo "  apctl        -> $(OUT_DIR)/apctl"
	@echo "  acctl-cli    -> $(OUT_DIR)/acctl-cli"

# Default rules for building individual targets
acser: libac.a $(AC_LIB_OBJS) $(AC_OBJS)
	@echo "  Linking acser..."
	$(CC) $(CFLAGS) -o $@ $^ -lpthread -lm -ljson-c

apctl: libac.a $(AP_OBJS)
	@echo "  Linking apctl..."
	$(CC) $(CFLAGS) -o $@ $^ -lpthread -lm

acctl-cli: ../ac/cli.o ../ac/db.o $(AC_LIB_OBJS) $(LIB_OBJS)
	@echo "  Linking acctl-cli..."
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $@ $^ -lpthread -lm -ljson-c

# Static library containing common code
libac.a: $(LIB_OBJS) $(AC_LIB_OBJS)
	@echo "  Creating libac.a..."
	ar rcs $@ $^

# Generic rules
%.o: %.c
	@echo "  CC $<"
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

../ac/%.o: ../ac/%.c
	@echo "  CC $<"
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

../ap/%.o: ../ap/%.c
	@echo "  CC $<"
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

clean:
	rm -f *.o ../ac/*.o ../ap/*.o acser apctl acctl-cli libac.a
	@echo "Clean complete."

# Dependencies
chap.o: chap.c $(INC_DIR)/chap.h $(INC_DIR)/msg.h $(INC_DIR)/log.h
md5.o: md5.c $(INC_DIR)/md5.h
link.o: link.c $(INC_DIR)/link.h $(INC_DIR)/log.h
thread.o: thread.c $(INC_DIR)/thread.h $(INC_DIR)/log.h
sec.o: sec.c $(INC_DIR)/sec.h $(INC_DIR)/log.h $(INC_DIR)/chap.h
dllayer.o: dllayer.c $(INC_DIR)/dllayer.h $(INC_DIR)/log.h
netlayer.o: netlayer.c $(INC_DIR)/netlayer.h $(INC_DIR)/log.h
arg.o: arg.c $(INC_DIR)/arg.h $(INC_DIR)/log.h
cfgarg.o: cfgarg.c
cmdarg.o: cmdarg.c $(INC_DIR)/arg.h $(INC_DIR)/log.h
mjson.o: mjson.c $(INC_DIR)/mjson.h

../ac/main.o: ../ac/main.c $(INC_DIR)/arg.h $(INC_DIR)/net.h \
	$(INC_DIR)/msg.h $(INC_DIR)/aphash.h $(INC_DIR)/process.h \
	$(INC_DIR)/netlayer.h $(INC_DIR)/resource.h $(INC_DIR)/sql.h \
	$(INC_DIR)/sec.h $(INC_DIR)/log.h
../ac/process.o: ../ac/process.c $(INC_DIR)/net.h $(INC_DIR)/log.h \
	$(INC_DIR)/msg.h $(INC_DIR)/aphash.h $(INC_DIR)/arg.h \
	$(INC_DIR)/netlayer.h $(INC_DIR)/apstatus.h $(INC_DIR)/link.h \
	$(INC_DIR)/process.h $(INC_DIR)/resource.h $(INC_DIR)/sec.h
../ac/net.o: ../ac/net.c $(INC_DIR)/dllayer.h $(INC_DIR)/aphash.h \
	$(INC_DIR)/net.h $(INC_DIR)/log.h $(INC_DIR)/msg.h \
	$(INC_DIR)/arg.h $(INC_DIR)/thread.h $(INC_DIR)/process.h \
	$(INC_DIR)/link.h $(INC_DIR)/chap.h
../ac/message.o: ../ac/message.c $(INC_DIR)/log.h $(INC_DIR)/arg.h \
	$(INC_DIR)/msg.h $(INC_DIR)/thread.h $(INC_DIR)/aphash.h \
	$(INC_DIR)/process.h $(INC_DIR)/apstatus.h
../ac/resource.o: ../ac/resource.c $(INC_DIR)/log.h $(INC_DIR)/resource.h \
	$(INC_DIR)/sql.h $(INC_DIR)/mjson.h $(INC_DIR)/db.h
../ac/db.o: ../ac/db.c $(INC_DIR)/db.h $(INC_DIR)/log.h $(INC_DIR)/mjson.h $(INC_DIR)/resource.h
../ac/cli.o: ../ac/cli.c $(INC_DIR)/db.h $(INC_DIR)/log.h $(INC_DIR)/sql.h
../ac/aphash.o: ../ac/aphash.c $(INC_DIR)/aphash.h $(INC_DIR)/log.h

../ap/main.o: ../ap/main.c $(INC_DIR)/arg.h $(INC_DIR)/message.h \
	$(INC_DIR)/process.h $(INC_DIR)/msg.h $(INC_DIR)/net.h \
	$(INC_DIR)/netlayer.h $(INC_DIR)/log.h $(INC_DIR)/link.h \
	$(INC_DIR)/thread.h $(INC_DIR)/sec.h $(INC_DIR)/aphash.h
../ap/process.o: ../ap/process.c $(INC_DIR)/msg.h $(INC_DIR)/log.h \
	$(INC_DIR)/dllayer.h $(INC_DIR)/netlayer.h $(INC_DIR)/link.h \
	$(INC_DIR)/net.h $(INC_DIR)/arg.h $(INC_DIR)/thread.h \
	$(INC_DIR)/process.h $(INC_DIR)/apstatus.h $(INC_DIR)/sec.h
../ap/message.o: ../ap/message.c $(INC_DIR)/log.h $(INC_DIR)/arg.h \
	$(INC_DIR)/msg.h $(INC_DIR)/thread.h $(INC_DIR)/process.h $(INC_DIR)/message.h
../ap/net.o: ../ap/net.c $(INC_DIR)/dllayer.h $(INC_DIR)/log.h \
	$(INC_DIR)/msg.h $(INC_DIR)/arg.h $(INC_DIR)/thread.h \
	$(INC_DIR)/process.h $(INC_DIR)/link.h $(INC_DIR)/net.h
../ap/apstatus.o: ../ap/apstatus.c $(INC_DIR)/apstatus.h $(INC_DIR)/log.h