# Makefile for acctl-ac - AC Controller Server
include $(TOPDIR)/rules.mk

PKG_NAME:=acctl-ac
PKG_VERSION:=2.0
PKG_RELEASE:=1
PKG_MAINTAINER:=jianxi sun <ycsunjane@gmail.com>
PKG_DESCRIPTION:=AC Controller - AC Server daemon
PKGARCH:=all

include $(INCLUDE_DIR)/package.mk

define Package/acctl-ac
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=Access Controllers
  TITLE:=AC Controller - AC Server
  DEPENDS:=+libjson-c +libpthread +libubus +libiwinfo +libmicrohttpd +libopenssl +libuci-lua
PKG_LICENSE:=Apache-2.0
PKG_LICENSE_FILES:=LICENSE
  URL:=https://github.com/bhrq12/acctl
endef

define Package/acctl-ac/description
  AC Controller Server daemon.
  Manages APs via CAPWAP protocol with CHAP authentication.
  Includes acctl-cli for command-line management.
endef

define Package/acctl-ac/conffiles
/etc/config/acctl-ac
endef

define Build/Prepare
	$(CP) $(CURDIR)/../src $(PKG_BUILD_DIR)/src
	$(CP) ./src/Makefile.ac $(PKG_BUILD_DIR)/src/
	$(CP) ./files $(PKG_BUILD_DIR)/files
endef

define Build/Configure
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/src \
		-f Makefile.ac \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -DSERVER -I./include -I./include/capwap" \
		LDFLAGS="$(TARGET_LDFLAGS) -lpthread -lm -ljson-c -lmicrohttpd -lssl -lcrypto -lrt" \
		acser acctl-cli
endef

define Package/acctl-ac/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/acser $(1)/usr/bin/acser
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/acctl-cli $(1)/usr/bin/acctl-cli
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/files/etc/init.d/acctl-ac $(1)/etc/init.d/acctl-ac
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/files/etc/config/acctl-ac $(1)/etc/config/acctl-ac
	$(INSTALL_DIR) $(1)/etc/acctl-ac
endef

define Package/acctl-ac/postinst
#!/bin/sh
[ "$${IPKG_NO_SCRIPT}" = "1" ] && exit 0
[ -s "$${IPKG_INSTROOT}/lib/functions.sh" ] || exit 0
. $${IPKG_INSTROOT}/lib/functions.sh
$${IPKG_INSTROOT}/etc/init.d/acctl-ac enable
exit 0
endef

define Package/acctl-ac/prerm
#!/bin/sh
[ -s "$${IPKG_INSTROOT}/lib/functions.sh" ] || exit 0
. $${IPKG_INSTROOT}/lib/functions.sh
$${IPKG_INSTROOT}/etc/init.d/acctl-ac disable 2>/dev/null || true
$${IPKG_INSTROOT}/etc/init.d/acctl-ac stop 2>/dev/null || true
exit 0
endef

$(eval $(call BuildPackage,acctl-ac))