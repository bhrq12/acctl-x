include $(TOPDIR)/rules.mk

PKG_NAME:=acctl
PKG_VERSION:=2.0
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/acctl
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=Access Controllers
  TITLE:=AC Controller - AP Management System
  DEPENDS:=+libuci-lua +libjson-c +libpthread +libubus +libiwinfo
  URL:=https://github.com/bhrq12/acctl
  MAINTAINER:=jianxi sun <ycsunjane@gmail.com>
endef

define Package/acctl/description
  OpenWrt AC Controller v2.0

  Features:
  - AC server (acser) manages APs centrally via TCP + ETH broadcast
  - AP client (apctl) runs on each managed Access Point
  - CHAP authentication with UCI-stored passwords (no hardcoded secrets)
  - JSON file-based database for AP configuration and status
  - AP grouping and batch configuration
  - Alarm/event logging
  - Firmware OTA upgrade support
  - LuCI web management interface

  Requires: OpenWrt 18.06+
endef

define Package/acctl/conffiles
/etc/config/acctl
endef

define Build/Prepare
	$(CP) ./src   $(PKG_BUILD_DIR)/src
	$(CP) ./files $(PKG_BUILD_DIR)/files
	$(CP) ./luci  $(PKG_BUILD_DIR)/luci
	$(CP) ./luci24 $(PKG_BUILD_DIR)/luci24
endef

define Build/Configure
endef

define Build/Compile
	+$(MAKE) -C $(PKG_BUILD_DIR)/src \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -I$(PKG_BUILD_DIR)/src/include -Wall -Wextra" \
		LDFLAGS="$(TARGET_LDFLAGS) -lpthread -lm -ljson-c" \
		all
endef

define Package/acctl/install
	# Install binaries
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/acser $(1)/usr/bin/acser
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/apctl $(1)/usr/bin/apctl
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/acctl-cli $(1)/usr/bin/acctl-cli

	# Install init scripts
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/files/etc/init.d/acctl $(1)/etc/init.d/acctl
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/files/etc/init.d/apctl $(1)/etc/init.d/apctl

	# Install config files
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/files/etc/config/acctl $(1)/etc/config/acctl

	# Create database directory
	$(INSTALL_DIR) $(1)/etc/acctl
	touch $(1)/etc/acctl/ac.json

	# Install LuCI files for OpenWrt 18.06-21.02 (Lua)
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/controller
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/luci/applications/luci-app-acctl/luasrc/controller/acctl.lua \
		$(1)/usr/lib/lua/luci/controller/acctl.lua

	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/model/cbi/acctl
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/luci/applications/luci-app-acctl/model/cbi/acctl/alarms.lua \
		$(1)/usr/lib/lua/luci/model/cbi/acctl/alarms.lua
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/luci/applications/luci-app-acctl/model/cbi/acctl/ap_list.lua \
		$(1)/usr/lib/lua/luci/model/cbi/acctl/ap_list.lua
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/luci/applications/luci-app-acctl/model/cbi/acctl/firmware.lua \
		$(1)/usr/lib/lua/luci/model/cbi/acctl/firmware.lua
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/luci/applications/luci-app-acctl/model/cbi/acctl/general.lua \
		$(1)/usr/lib/lua/luci/model/cbi/acctl/general.lua
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/luci/applications/luci-app-acctl/model/cbi/acctl/groups.lua \
		$(1)/usr/lib/lua/luci/model/cbi/acctl/groups.lua
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/luci/applications/luci-app-acctl/model/cbi/acctl/system.lua \
		$(1)/usr/lib/lua/luci/model/cbi/acctl/system.lua
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/luci/applications/luci-app-acctl/model/cbi/acctl/templates.lua \
		$(1)/usr/lib/lua/luci/model/cbi/acctl/templates.lua

	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/view/acctl
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/luci/applications/luci-app-acctl/view/acctl/alarm_level_badge.htm \
		$(1)/usr/lib/lua/luci/view/acctl/alarm_level_badge.htm
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/luci/applications/luci-app-acctl/view/acctl/ap_band.htm \
		$(1)/usr/lib/lua/luci/view/acctl/ap_band.htm
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/luci/applications/luci-app-acctl/view/acctl/ap_info.htm \
		$(1)/usr/lib/lua/luci/view/acctl/ap_info.htm
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/luci/applications/luci-app-acctl/view/acctl/ap_status.htm \
		$(1)/usr/lib/lua/luci/view/acctl/ap_status.htm
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/luci/applications/luci-app-acctl/view/acctl/ap_status_badge.htm \
		$(1)/usr/lib/lua/luci/view/acctl/ap_status_badge.htm
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/luci/applications/luci-app-acctl/view/acctl/ap_status_info.htm \
		$(1)/usr/lib/lua/luci/view/acctl/ap_status_info.htm
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/luci/applications/luci-app-acctl/view/acctl/ap_wifi.htm \
		$(1)/usr/lib/lua/luci/view/acctl/ap_wifi.htm
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/luci/applications/luci-app-acctl/view/acctl/status_bar.htm \
		$(1)/usr/lib/lua/luci/view/acctl/status_bar.htm
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/luci/applications/luci-app-acctl/view/acctl/user_badge.htm \
		$(1)/usr/lib/lua/luci/view/acctl/user_badge.htm

	# Install LuCI files for OpenWrt 22+ (ucode)
	$(INSTALL_DIR) $(1)/usr/share/ucode/luci/controller
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/luci24/usr/share/ucode/luci/controller/acctl.js \
		$(1)/usr/share/ucode/luci/controller/acctl.js

	$(INSTALL_DIR) $(1)/usr/share/luci/views/acctl
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/luci24/usr/share/luci/views/acctl/*.ut \
		$(1)/usr/share/luci/views/acctl/

	$(INSTALL_DIR) $(1)/usr/share/rpcd/acl.d
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/luci24/usr/share/rpcd/acl.d/luci-app-acctl.json \
		$(1)/usr/share/rpcd/acl.d/luci-app-acctl.json
endef

define Package/acctl/postinst
#!/bin/sh
[ "$${IPKG_NO_SCRIPT}" = "1" ] && exit 0
[ -s "$${IPKG_INSTROOT}/lib/functions.sh" ] || exit 0
. $${IPKG_INSTROOT}/lib/functions.sh

# Initialize database directory
[ -d $${IPKG_INSTROOT}/etc/acctl ] || mkdir -p $${IPKG_INSTROOT}/etc/acctl

# Create default database if not exists
if [ ! -f $${IPKG_INSTROOT}/etc/acctl/ac.json ]; then
cat > $${IPKG_INSTROOT}/etc/acctl/ac.json <<'EOF'
{
  "ac": {
    "uuid": "ac-$(cat /proc/sys/kernel/random/uuid)",
    "name": "OpenWrt-AC"
  },
  "resource": {
    "ip_start": "192.168.1.200",
    "ip_end": "192.168.1.254",
    "ip_mask": "255.255.255.0"
  },
  "aps": [],
  "groups": [],
  "alarms": [],
  "firmwares": []
}
EOF
fi

# Enable and start the service
$${IPKG_INSTROOT}/etc/init.d/acctl enable
$${IPKG_INSTROOT}/etc/init.d/acctl start

exit 0
endef

$(eval $(call BuildPackage,acctl))
