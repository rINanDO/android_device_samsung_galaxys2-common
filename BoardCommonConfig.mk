#
# Copyright (C) 2012 The Android Open-Source Project
# Copyright (C) 2012 The CyanogenMod Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

DEVICE_PATH := device/samsung/galaxys2-common

# HIDL
PRODUCT_ENFORCE_VINTF_MANIFEST_OVERRIDE := true

# Allow duplicate rules to override them
BUILD_BROKEN_DUP_RULES := true

# PRODUCT_COPY_FILES directives.
BUILD_BROKEN_ELF_PREBUILT_PRODUCT_COPY_FILES := true

# This variable is set first, so it can be overridden
# by BoardConfigVendor.mk
BOARD_USES_GENERIC_AUDIO := false
BOARD_USES_LEGACY_MMAP := true
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_CPU_SMP := true
TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv7-a-neon
TARGET_ARCH_VARIANT_CPU := cortex-a9
TARGET_CPU_VARIANT := cortex-a9
ARCH_ARM_HAVE_NEON := true
ARCH_ARM_HAVE_TLS_REGISTER := true
TARGET_USES_GRALLOC1 := true
TARGET_USES_64_BIT_BINDER := true

BOARD_VENDOR := samsung
TARGET_BOARD_PLATFORM := exynos4
TARGET_SOC := exynos4210
TARGET_BOOTLOADER_BOARD_NAME := smdk4210

TARGET_NO_BOOTLOADER := true
TARGET_NO_RADIOIMAGE := true
TARGET_NO_SEPARATE_RECOVERY := true

TARGET_PROVIDES_INIT := true
TARGET_PROVIDES_INIT_TARGET_RC := true

#BOARD_NAND_PAGE_SIZE := 4096
#BOARD_NAND_SPARE_SIZE := 128
BOARD_KERNEL_PAGESIZE := 4096
BOARD_KERNEL_BASE := 0x40000000
BOARD_KERNEL_CMDLINE := console=ttySAC2,115200 consoleblank=0
BOARD_KERNEL_IMAGE_NAME := zImage
NEED_KERNEL_MODULE_SYSTEM := true
TARGET_KERNEL_CLANG_COMPILE := false

# Manifest
DEVICE_MANIFEST_FILE := $(DEVICE_PATH)/manifest.xml
DEVICE_FRAMEWORK_COMPATIBILITY_MATRIX_FILE += $(DEVICE_PATH)/framework_compatibility_matrix.xml

# Properties
TARGET_VENDOR_PROP += $(COMMON_PATH)/vendor.prop

# Bionic
# The linker splits the entries on ':', so keep them on one line
SHIM_LIBS := \
    /system/vendor/lib/libsec-ril.so|libcutils-s2-shim.so \
    /system/vendor/lib/hw/gps.exynos4.vendor.so|libcutils-s2-shim.so
$(call soong_config_set,bionic,ld_shim_libs,"$(subst $(space),:,$(strip $(SHIM_LIBS)))")

SDK_VERSION_OVERRIDES := \
    /vendor/bin/hw/rild=22 \
    /system/vendor/lib/libsec-ril.so=22 \
    /system/lib/libsecnativefeature.so=22 \
    /system/lib/libomission_avoidance.so=22 \
    /system/lib/libfactoryutil.so=22 \
    /system/vendor/lib/libakm.so=22 \
    /system/vendor/lib/libsecril-client.so=22 \
    /system/vendor/lib/libsecril-cl-gps.so=22 \
    /system/vendor/lib/hw/gps.exynos4.vendor.so=22
$(call soong_config_set,bionic,sdk_version_overrides,"$(strip $(SDK_VERSION_OVERRIDES))")

# Include an expanded selection of fonts
EXTENDED_FONT_FOOTPRINT := true

# Memory management
MALLOC_SVELTE := true

# Filesystem
TARGET_FS_CONFIG_GEN := device/samsung/galaxys2-common/config.fs
TARGET_USERIMAGES_USE_EXT4 := true
TARGET_USERIMAGES_USE_F2FS := true
BOARD_BOOTIMAGE_PARTITION_SIZE := 8388608
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 2147483648
BOARD_USERDATAIMAGE_PARTITION_SIZE := 2147467264
BOARD_CACHEIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_CACHEIMAGE_PARTITION_SIZE := 104857600
BOARD_FLASH_BLOCK_SIZE := 4096
BOARD_ROOT_EXTRA_FOLDERS := efs misc
BOARD_ROOT_EXTRA_SYMLINKS := /data/tombstones:/tombstones

# Releasetools
TARGET_RELEASETOOLS_EXTENSIONS := $(DEVICE_PATH)

# Hardware tunables
BOARD_HARDWARE_CLASS := hardware/samsung/lineagehw \
    device/samsung/galaxys2-common/lineagehw

# Graphics
BOARD_EGL_CFG := device/samsung/galaxys2-common/configs/egl.cfg
USE_OPENGL_RENDERER := true
TARGET_RUNNING_WITHOUT_SYNC_FRAMEWORK := true

# FIMG Acceleration
BOARD_USES_FIMGAPI := true
BOARD_USES_SKIA_FIMGAPI := true

# Enable WEBGL in WebKit
ENABLE_WEBGL := true

# HWComposer
BOARD_USES_HWCOMPOSER := true
BOARD_USE_SYSFS_VSYNC_NOTIFICATION := true
NUM_FRAMEBUFFER_SURFACE_BUFFERS := 3

# OMX
BOARD_NONBLOCK_MODE_PROCESS := true
BOARD_USE_STOREMETADATA := true
BOARD_USE_METADATABUFFERTYPE := true
BOARD_USES_MFC_FPS := true
BOARD_USE_S3D_SUPPORT := true
BOARD_USE_CSC_FIMC := false

# Audio
BOARD_USE_TINYALSA_AUDIO := true
BOARD_USE_YAMAHA_MC1N2_AUDIO := true
USE_XML_AUDIO_POLICY_CONF := 1

# RIL
BOARD_PROVIDES_LIBRIL := true
BOARD_MODEM_TYPE := xmm6260
BOARD_RIL_CLASS := ../../../device/samsung/galaxys2-common/ril
$(call soong_config_set,ril,samsung_libril,true)
$(call soong_config_set,ril,samsung_modem_type,$(BOARD_MODEM_TYPE))

# Camera
BOARD_CAMERA_HAVE_ISO := true

# WiFi
BOARD_WLAN_DEVICE := bcmdhd
BOARD_HAVE_SAMSUNG_WIFI := true
BOARD_HOSTAPD_DRIVER := NL80211
BOARD_HOSTAPD_PRIVATE_LIB := lib_driver_cmd_${BOARD_WLAN_DEVICE}
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_${BOARD_WLAN_DEVICE}
WPA_SUPPLICANT_VERSION := VER_0_8_X
WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/dhd/parameters/firmware_path"
WIFI_DRIVER_FW_PATH_STA     := "/system/vendor/etc/wifi/bcmdhd_sta.bin"
WIFI_DRIVER_FW_PATH_AP      := "/system/vendor/etc/wifi/bcmdhd_apsta.bin"
WIFI_DRIVER_FW_PATH_P2P     := "/system/vendor/etc/wifi/bcmdhd_p2p.bin"
WIFI_HIDL_UNIFIED_SUPPLICANT_SERVICE_RC_ENTRY := true

# Network Routing
$(call soong_config_set,netd,direct_connect_rule,true)

# Bluetooth
BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_BCM := true
PRODUCT_SOONG_NAMESPACES += hardware/broadcom/libbt
$(call soong_config_set,brcm_libbt,custom_bt_config,//$(DEVICE_PATH)/bluetooth:vnd_smdk4210.txt)

# Selinux
SELINUX_IGNORE_NEVERALLOWS := true
BOARD_VENDOR_SEPOLICY_DIRS += device/samsung/galaxys2-common/selinux/vendor
SYSTEM_EXT_PUBLIC_SEPOLICY_DIRS += device/samsung/galaxys2-common/selinux/public
SYSTEM_EXT_PRIVATE_SEPOLICY_DIRS += device/samsung/galaxys2-common/selinux/private

# Recovery
BOARD_CUSTOM_RECOVERY_KEYMAPPING := ../../device/samsung/galaxys2-common/recovery/recovery_keys.c
BOARD_CUSTOM_GRAPHICS := ../../../device/samsung/galaxys2-common/recovery/graphics.c
BOARD_UMS_LUNFILE := "/sys/class/android_usb/android0/f_mass_storage/lun%d/file"
BOARD_USES_MMCUTILS := true
BOARD_USES_FULL_RECOVERY_IMAGE := true
BOARD_NO_RECOVERY_PATCH := true
BOARD_HAS_NO_MISC_PARTITION := true
BOARD_HAS_NO_SELECT_BUTTON := true
BOARD_SUPPRESS_EMMC_WIPE := true
BOARD_RECOVERY_SWIPE := true
TARGET_RECOVERY_DENSITY := mdpi
RECOVERY_FSTAB_VERSION := 2

# Device specific headers
TARGET_SPECIFIC_HEADER_PATH := device/samsung/galaxys2-common/include

# Charging mode
BOARD_BATTERY_DEVICE_NAME := "battery"
BOARD_CHARGER_SHOW_PERCENTAGE := true
WITH_LINEAGE_CHARGER := false

# Boot.img
BOARD_CUSTOM_BOOTIMG := true
BOARD_CUSTOM_BOOTIMG_MK := device/samsung/galaxys2-common/shbootimg.mk
BOARD_CUSTOM_KERNEL_MK := device/samsung/galaxys2-common/shkernel.mk

# Memfd
$(call soong_config_set,art,has_memfd_backport,true)

# Use the non-open-source parts, if they're present
-include vendor/samsung/galaxys2-common/BoardConfigVendor.mk
