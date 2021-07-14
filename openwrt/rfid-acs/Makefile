
include $(TOPDIR)/rules.mk

PKG_NAME:=rfid-acs
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/rfid-acs
  $(call Package/i2c/Default)
  CATEGORY:=Gemtek packages
  SUBMENU:=Packages
  TITLE+=RFID door control system
endef

define Package/rfid-acs/description
 A door control system using RDM6300 125KHz card reader
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef

TARGET_CFLAGS += -I$(PKG_BUILD_DIR)/include
#TARGET_CFLAGS += -DDEBUG

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) 		\
		LINUX="$(LINUX_DIR)" 			\
		CC="$(TARGET_CC)" 				\
		STAGING_DIR="$(STAGING_DIR)" 	\
		LDFLAGS="$(TARGET_LDFLAGS)" 	\
		CFLAGS="$(TARGET_CFLAGS)"
endef

define Package/rfid-acs/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/rfid-acs	$(1)/usr/sbin
	$(INSTALL_BIN) files/eip-sign.sh			$(1)/usr/sbin
endef

$(eval $(call BuildPackage,rfid-acs))
