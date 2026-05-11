# Makefile for acctl-ap - AC Controller AP Client
include $(TOPDIR)/rules.mk

PKG_NAME:=acctl-ap
PKG_VERSION:=2.0
PKG_RELEASE:=1
PKG_MAINTAINER:=jianxi sun <ycsunjane@gmail.com>
PKG_DESCRIPTION:=AC Controller - AP Client for AC management
PKGARCH:=all

include $(INCLUDE_DIR)/package.mk

define Package/acctl-ap
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=Access Controllers
  TITLE:=AC Controller - AP Client
  DEPENDS:=+libjson-c +libpthread +libubus +libiwinfo +libopenssl
PKG_LICENSE:=Apache-2.0
PKG_LICENSE_FILES:=LICENSE
  URL:=https://github.com/bhrq12/acctl
endef

define Package/acctl-ap/description
  AP Client for AC Controller.
  Connects to AC server via CAPWAP protocol for centralized management.
endef

define Package/acctl-ap/conffiles
/etc/config/acctl-ap
endef

define Build/Prepare
	$(CP) $(CURDIR)/../src $(PKG_BUILD_DIR)/src
	$(CP) ./src/Makefile.ap $(PKG_BUILD_DIR)/src/
	$(CP) ./files $(PKG_BUILD_DIR)/files
endef

define Build/Configure
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/src \
		-f Makefile.ap \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -DCLIENT -I./include -I./include/capwap" \
		LDFLAGS="$(TARGET_LDFLAGS) -lpthread -lm -ljson-c -lssl -lcrypto -lrt" \
		CLIENT=1 SERVER= CLI= \
		apctl
endef

define Package/acctl-ap/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/apctl $(1)/usr/bin/apctl
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/files/etc/init.d/acctl-ap $(1)/etc/init.d/acctl-ap
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/files/etc/config/acctl-ap $(1)/etc/config/acctl-ap
	$(INSTALL_DIR) $(1)/etc/acctl-ap
endef

define Package/acctl-ap/postinst
#!/bin/sh
[ "$${IPKG_NO_SCRIPT}" = "1" ] && exit 0
[ -s "$${IPKG_INSTROOT}/lib/functions.sh" ] || exit 0
. $${IPKG_INSTROOT}/lib/functions.sh
$${IPKG_INSTROOT}/etc/init.d/acctl-ap enable
exit 0
endef

define Package/acctl-ap/prerm
#!/bin/sh
[ -s "$${IPKG_INSTROOT}/lib/functions.sh" ] || exit 0
. $${IPKG_INSTROOT}/lib/functions.sh
$${IPKG_INSTROOT}/etc/init.d/acctl-ap disable 2>/dev/null || true
$${IPKG_INSTROOT}/etc/init.d/acctl-ap stop 2>/dev/null || true
exit 0
endef

$(eval $(call BuildPackage,acctl-ap))