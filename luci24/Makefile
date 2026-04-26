#
# AC Controller - OpenWrt 24.x Compatibility Package
# Supports OpenWrt 22.03+ with ucode-based LuCI
#

include $(TOPDIR)/rules.mk

PKG_NAME:=luci-app-acctl
PKG_VERSION:=2.0
PKG_RELEASE:=1
PKG_LICENSE:=Apache-2.0

PKG_DEPENDS:=+luci-lib-base +luci-lib-jsonc +libuci-lua +libjson-c +libpthread +libubus +libiwinfo

include $(INCLUDE_DIR)/package.mk

define Package/luci-app-acctl
  SECTION:=luci
  CATEGORY:=LuCI
  SUBMENU:=Access Controllers
  TITLE:=AC Controller - AP Management System
  DEPENDS:=$(PKG_DEPENDS)
  URL:=https://github.com/bhrq12/acctl
  MAINTAINER:=jianxi sun <ycsunjane@gmail.com>
  PKGARCH:=all
endef

define Package/luci-app-acctl/description
 OpenWrt AC Controller v2.0 - Web Management Interface

 Compatible with OpenWrt 22.03+ and LuCI with ucode support.

 Features:
 - AC server (acser) manages APs centrally via TCP + ETH broadcast
 - AP client (apctl) runs on each managed Access Point
 - CHAP authentication with UCI-stored passwords
 - JSON file-based database for AP configuration and status
 - AP grouping and batch configuration
 - Alarm/event logging
 - Firmware OTA upgrade support

 Requires: acctl package (acser, apctl, acctl-cli)
endef

define Build/Prepare
	$(CP) ./src      $(PKG_BUILD_DIR)/src
	$(CP) ./luci24   $(PKG_BUILD_DIR)/luci24
	$(CP) ./files    $(PKG_BUILD_DIR)/files
endef

define Build/Configure
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/src \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -I$(PKG_BUILD_DIR)/src/include -Wall -Wextra" \
		LDFLAGS="$(TARGET_LDFLAGS) -lpthread -lm -ljson-c" \
		all
endef

define Package/luci-app-acctl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/acser $(1)/usr/bin/acser
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/apctl $(1)/usr/bin/apctl
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/acctl-cli $(1)/usr/bin/acctl-cli

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/files/etc/init.d/acctl $(1)/etc/init.d/acctl
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/files/etc/init.d/apctl $(1)/etc/init.d/apctl

	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/files/etc/config/acctl $(1)/etc/config/acctl

	$(INSTALL_DIR) $(1)/etc/acctl
	touch $(1)/etc/acctl/ac.json

	$(INSTALL_DIR) $(1)/usr/share/ucode/luci/controller
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/luci24/usr/share/ucode/luci/controller/acctl.js \
		$(1)/usr/share/ucode/luci/controller/acctl.js

	$(INSTALL_DIR) $(1)/usr/share/luci/views/acctl
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/luci24/usr/share/luci/views/acctl/*.ut \
		$(1)/usr/share/luci/views/acctl/

	$(INSTALL_DIR) $(1)/usr/share/rpcd/acl.d
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/luci24/usr/share/rpcd/acl.d/luci-app-acctl.json \
		$(1)/usr/share/rpcd/acl.d/luci-app-acctl.json

	$(INSTALL_DIR) $(1)/etc/uci-defaults
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/files/etc/uci-defaults/luci-app-acctl \
		$(1)/etc/uci-defaults/luci-app-acctl
endef

$(eval $(call BuildPackage,luci-app-acctl))