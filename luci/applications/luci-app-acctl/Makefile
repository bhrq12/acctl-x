# Makefile for luci-app-acctl
include $(TOPDIR)/rules.mk

LUCI_TITLE:=LuCI - AC Controller
LUCI_DEPENDS:=+luci-base +luci-lib-base +libuci-lua +luci-compat +acctl
LUCI_PKGARCH:=all

include ../../luci.mk

PKG_VERSION:=2.0
PKG_RELEASE:=1

define Package/luci-app-acctl/description
  LuCI web interface for AC Controller
  Provides centralized AP management through web browser.
endef
