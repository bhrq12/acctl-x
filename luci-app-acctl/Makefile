# Makefile for luci-app-acctl - LuCI Web Interface for AC Controller

include $(TOPDIR)/rules.mk

PKG_NAME:=luci-app-acctl
PKG_VERSION:=2.0
PKG_RELEASE:=1
PKG_MAINTAINER:=jianxi sun <ycsunjane@gmail.com>

include $(INCLUDE_DIR)/package.mk

define Package/luci-app-acctl
  SECTION:=luci
  CATEGORY:=LuCI
  SUBMENU:=Access Controllers
  TITLE:=AC Controller
  DEPENDS:=+luci-base +luci-lib-base +libuci-lua +luci-compat +acctl-ac
  PKGARCH:=all
endef

define Package/luci-app-acctl/description
  Web management interface for AC Controller
endef

LUCI_TITLE:=AC Controller
LUCI_DESCRIPTION:=Web management interface for AC Controller
LUCI_DEPENDS:=+luci-base +luci-lib-base +libuci-lua +luci-compat +acctl-ac
PKG_LICENSE:=Apache-2.0
PKG_LICENSE_FILES:=LICENSE
LUCI_PKGARCH:=all
LUCI_JAVASCRIPT:= \
	luasrc/js/acctl/compat.js \
	luasrc/js/acctl/api.js \
	luasrc/js/acctl/main.js
LUCI_CSS:= \
	luasrc/css/acctl/style.css
LUCI_LIBRARY:= \
	luasrc/lib/lua/

include $(TOPDIR)/feeds/luci/luci.mk

$(eval $(call BuildPackage,luci-app-acctl))