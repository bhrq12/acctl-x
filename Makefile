include $(TOPDIR)/rules.mk

PKG_NAME:=acctl
PKG_VERSION:=2.0
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/acctl
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=Access Controllers
  TITLE:=AC Controller - Complete Package
  DEPENDS:=+acctl-ac +acctl-ap +luci-app-acctl
  URL:=https://github.com/bhrq12/acctl
  MAINTAINER:=jianxi sun <ycsunjane@gmail.com>
  PKGARCH:=all
endef

define Package/acctl/description
  AC Controller - Complete Package
  Includes AC server, AP client, and LuCI web interface
  
  Components:
  - acctl-ac: AC Server (acser) - Central controller
  - acctl-ap: AP Client (apctl) - Runs on managed APs
  - luci-app-acctl: Web management interface
  
  Features:
  - Centralized AP management via TCP + ETH broadcast
  - CHAP authentication with UCI-stored passwords
  - JSON file-based database for configuration and status
  - AP grouping and batch configuration
  - Alarm/event logging
  - Firmware OTA upgrade support
  - Multi-SSID support
  - Profile-based configuration templates
  - Real-time status monitoring
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/acctl/install
	# Meta package - no files to install
endef

$(eval $(call BuildPackage,acctl))

# Include individual package makefiles
-include $(CURDIR)/Makefile.ac
-include $(CURDIR)/Makefile.ap
-include $(CURDIR)/Makefile.luci