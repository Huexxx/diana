#Android makefile to build kernel as a part of Android Build

ifeq ($(TARGET_PREBUILT_KERNEL),)

KERNEL_OUT := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ
KERNEL_CONFIG := $(KERNEL_OUT)/.config
TARGET_PREBUILT_KERNEL := kernel/arch/arm/boot/zImage
KERNEL_MODULES_OUT := $(TARGET_OUT)/etc/wifi
TARGET_PREBUILT_INT_KERNEL := $(KERNEL_OUT)/arch/arm/boot/zImage
WIFI_KERNEL_MODULE_NAME := wireless.ko
WIFI_KERNEL_MODULE := $(KERNEL_OUT)/drivers/net/wireless/bcm4329/$(WIFI_KERNEL_MODULE_NAME)

ifeq ($(TARGET_USES_UNCOMPRESSED_KERNEL),true)
$(info Using uncompressed kernel)
TARGET_PREBUILT_KERNEL := $(KERNEL_OUT)/piggy
else
TARGET_PREBUILT_KERNEL := $(TARGET_PREBUILT_INT_KERNEL)
endif

KERNEL_DEFCONFIG_SRC_FILE := kernel/arch/arm/configs/$(KERNEL_DEFCONFIG)

$(KERNEL_OUT):
	mkdir -p $(KERNEL_OUT)

$(KERNEL_MODULES_OUT):
	mkdir -p $(KERNEL_MODULES_OUT)
	mkdir -p $(TARGET_OUT_EXECUTABLES)

$(KERNEL_CONFIG): $(KERNEL_DEFCONFIG_SRC_FILE) | $(KERNEL_OUT)
	$(MAKE) -C kernel O=../$(KERNEL_OUT) ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi- $(KERNEL_DEFCONFIG)

$(KERNEL_OUT)/piggy : $(TARGET_PREBUILT_INT_KERNEL)
	$(hide) gunzip -c $(KERNEL_OUT)/arch/arm/boot/compressed/piggy > $(KERNEL_OUT)/piggy

.PHONY: $(TARGET_PREBUILT_INT_KERNEL)
$(TARGET_PREBUILT_INT_KERNEL): $(KERNEL_CONFIG)
	+$(MAKE) -C kernel O=../$(KERNEL_OUT) ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi- zImage

.PHONY: $(WIFI_KERNEL_MODULE)
$(WIFI_KERNEL_MODULE): $(TARGET_PREBUILT_INT_KERNEL) | $(KERNEL_MODULES_OUT)
	+$(MAKE) -C kernel O=../$(KERNEL_OUT) ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi- modules


PRODUCT_COPY_FILES_TO := $(subst $(PRODUCT_OUT),,$(KERNEL_MODULES_OUT))
PRODUCT_COPY_FILES += \
	$(WIFI_KERNEL_MODULE):$(PRODUCT_COPY_FILES_TO)/$(WIFI_KERNEL_MODULE_NAME)\
	kernel/drivers/net/wireless/bcm4329/firmware/fw_bcm4329.bin:$(PRODUCT_COPY_FILES_TO)/fw_bcm4329.bin\
	kernel/drivers/net/wireless/bcm4329/firmware/fw_bcm4329_mfg.bin:$(PRODUCT_COPY_FILES_TO)/fw_bcm4329_mfg.bin\
	kernel/drivers/net/wireless/bcm4329/firmware/fw_bcm4329_ap.bin:$(PRODUCT_COPY_FILES_TO)/fw_bcm4329_ap.bin\
	kernel/drivers/net/wireless/bcm4329/firmware/sdio-g-cdc-roml-reclaim-full11n-wme-idsup-idauth-p2p-memredux16.bin:$(PRODUCT_COPY_FILES_TO)/fw_bcm4329_p2p.bin\
	kernel/drivers/net/wireless/bcm4329/firmware/nvram.txt:$(PRODUCT_COPY_FILES_TO)/nvram.txt\
	kernel/drivers/net/wireless/bcm4329/wpa_supplicant.conf:$(PRODUCT_COPY_FILES_TO)/wpa_supplicant.conf

PRODUCT_COPY_FILES_TO := $(subst $(PRODUCT_OUT),,$(TARGET_OUT_EXECUTABLES))
PRODUCT_COPY_FILES += \
	kernel/drivers/net/wireless/bcm4329/wl:$(PRODUCT_COPY_FILES_TO)/wl


kerneltags: $(KERNEL_OUT) $(KERNEL_CONFIG)
	$(MAKE) -C kernel ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi- tags

kernelconfig: $(KERNEL_OUT) $(KERNEL_CONFIG)
	env KCONFIG_NOTIMESTAMP=true \
	     $(MAKE) -C kernel ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi- menuconfig
	cp $(KERNEL_OUT)/.config kernel/arch/arm/configs/$(KERNEL_DEFCONFIG)


INSTALLED_KERNEL_TARGET ?= $(PRODUCT_OUT)/kernel
INSTALLED_BOOTIMAGE_TARGET ?= $(PRODUCT_OUT)/boot.img
INSTALLED_RECOVERYIMAGE_TARGET ?= $(PRODUCT_OUT)/recovery.img

KERNEL_CLEAN_TARGET_LIST := $(TARGET_PREBUILT_INT_KERNEL) $(TARGET_PREBUILT_KERNEL) $(INSTALLED_KERNEL_TARGET) \
                            $(INSTALLED_BOOTIMAGE_TARGET) $(INSTALLED_RECOVERYIMAGE_TARGET)

.PHONY: ckernel
ckernel:
	-@$(MAKE) -C kernel O=../$(KERNEL_OUT) ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi- distclean
	-@rm -vf $(KERNEL_CLEAN_TARGET_LIST)

.PHONY: kernel
kernel: $(INSTALLED_BOOTIMAGE_TARGET) $(INSTALLED_RECOVERYIMAGE_TARGET)
$(INSTALLED_BOOTIMAGE_TARGET): $(INSTALLED_KERNEL_TARGET)
$(INSTALLED_RECOVERYIMAGE_TARGET): $(INSTALLED_KERNEL_TARGET)
$(INSTALLED_KERNEL_TARGET): $(TARGET_PREBUILT_INT_KERNEL)

endif
