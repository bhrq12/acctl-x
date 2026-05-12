# Makefile for acctl-py - AC Controller Python Implementation
include $(TOPDIR)/rules.mk

PKG_NAME:=acctl-py
PKG_VERSION:=1.0
PKG_RELEASE:=3
PKG_MAINTAINER:=jianxi sun <ycsunjane@gmail.com>
PKG_DESCRIPTION:=AC Controller - Python Implementation

include $(INCLUDE_DIR)/package.mk
include $(TOPDIR)/feeds/packages/lang/python/python3-package.mk

define Package/acctl-py
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=Access Controllers
  TITLE:=AC Controller (Python)
  DEPENDS:=+python3-light
  URL:=https://github.com/bhrq12/acctl
endef

define Package/acctl-py/description
  AC Controller Server daemon implemented in Python.
  Manages APs via CAPWAP protocol with CHAP authentication.
  Includes REST API and CLI management tools.

  Compatible with Python 3.6+ and OpenWrt 19.07+.
endef

define Package/acctl-py/conffiles
/etc/config/acctl-py
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src $(PKG_BUILD_DIR)/
	$(CP) ./files $(PKG_BUILD_DIR)/
endef

define Build/Compile
	# Python packages don't need compilation
endef

define Package/acctl-py/install
	# Install Python modules to standard site-packages path
	$(INSTALL_DIR) $(1)/usr/lib/python$(PYTHON3_VERSION)/site-packages/acctl
	$(CP) $(PKG_BUILD_DIR)/src/acctl/* $(1)/usr/lib/python$(PYTHON3_VERSION)/site-packages/acctl/

	# Install CLI script
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/files/usr/bin/acctl-py-cli $(1)/usr/bin/acctl-py-cli

	# Install init script
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/files/etc/init.d/acctl-py $(1)/etc/init.d/acctl-py

	# Install config file
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/files/etc/config/acctl-py $(1)/etc/config/acctl-py

	# Create data directory
	$(INSTALL_DIR) $(1)/etc/acctl-py
endef

define Package/acctl-py/postinst
#!/bin/sh
[ "$${IPKG_NO_SCRIPT}" = "1" ] && exit 0
[ -s "$${IPKG_INSTROOT}/lib/functions.sh" ] || exit 0
. $${IPKG_INSTROOT}/lib/functions.sh
$${IPKG_INSTROOT}/etc/init.d/acctl-py enable
exit 0
endef

define Package/acctl-py/prerm
#!/bin/sh
[ -s "$${IPKG_INSTROOT}/lib/functions.sh" ] || exit 0
. $${IPKG_INSTROOT}/lib/functions.sh
$${IPKG_INSTROOT}/etc/init.d/acctl-py disable 2>/dev/null || true
$${IPKG_INSTROOT}/etc/init.d/acctl-py stop 2>/dev/null || true
exit 0
endef

$(eval $(call BuildPackage,acctl-py))