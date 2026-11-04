#
# Copyright (C) 2012 The Android Open-Source Project
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
COMMON_PATH := device/samsung/galaxys2-common

PRODUCT_BUILD_RECOVERY_IMAGE := true

# Enable DM file pre-opting to reduce first boot time
PRODUCT_DEX_PREOPT_GENERATE_DM_FILES := true
PRODUCT_DEX_PREOPT_DEFAULT_COMPILER_FILTER := verify

DONT_UNCOMPRESS_PRIV_APPS_DEXS := true

DEVICE_PACKAGE_OVERLAYS := $(COMMON_PATH)/overlay

# RRO (Runtime Resource Overlay)
PRODUCT_ENFORCE_RRO_TARGETS += *
PRODUCT_ENFORCE_RRO_EXCLUDED_OVERLAYS  += * \
    $(COMMON_PATH)/overlay-lineage/lineage-sdk \
    $(COMMON_PATH)/overlay/hardware

# Init
PRODUCT_COPY_FILES := \
    $(COMMON_PATH)/rootdir/init.smdk4210.usb.rc:root/init.smdk4210.usb.rc \
    $(COMMON_PATH)/rootdir/init.smdk4210.rc:root/init.smdk4210.rc

# init.d
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/tweaks.rc:system/vendor/etc/init/tweaks.rc

# File system table
PRODUCT_COPY_FILES += \
    $(COMMON_PATH)/rootdir/fstab.smdk4210:root/fstab.smdk4210 \
    $(COMMON_PATH)/rootdir/fstab.smdk4210:$(TARGET_COPY_OUT_RAMDISK)/fstab.smdk4210 \
    $(COMMON_PATH)/rootdir/fstab.smdk4210:$(TARGET_COPY_OUT_VENDOR)/etc/fstab.smdk4210

# Audio effects
PRODUCT_COPY_FILES += \
    frameworks/av/media/libeffects/data/audio_effects.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_effects.xml

# Component overrides
PRODUCT_COPY_FILES += \
    $(COMMON_PATH)/configs/component-overrides.xml:$(TARGET_COPY_OUT_VENDOR)/etc/sysconfig/component-overrides.xml

# TWRP
ifeq ($(RECOVERY_VARIANT),twrp)
$(call inherit-product, device/samsung/galaxys2-common/twrp/twrp.mk)
else
TARGET_RECOVERY_FSTAB := device/samsung/galaxys2-common/rootdir/fstab.smdk4210
endif


# Recovery rootdir
PRODUCT_COPY_FILES += \
    $(COMMON_PATH)/rootdir/init.recovery.smdk4210.rc:root/init.recovery.smdk4210.rc

# Audio
PRODUCT_COPY_FILES += \
    $(COMMON_PATH)/configs/tinyalsa-audio.xml:$(TARGET_COPY_OUT_VENDOR)/etc/tinyalsa-audio.xml \
    $(COMMON_PATH)/configs/audio_policy/audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_configuration.xml \
    frameworks/av/services/audiopolicy/config/bluetooth_audio_policy_configuration_7_0.xml:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth_audio_policy_configuration_7_0.xml \
    $(COMMON_PATH)/configs/audio_policy/usb_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/usb_audio_policy_configuration.xml \
    $(COMMON_PATH)/configs/audio_policy/r_submix_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/r_submix_audio_policy_configuration.xml

PRODUCT_COPY_FILES += \
    frameworks/av/services/audiopolicy/config/audio_policy_volumes.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_volumes.xml \
    frameworks/av/services/audiopolicy/config/default_volume_tables.xml:$(TARGET_COPY_OUT_VENDOR)/etc/default_volume_tables.xml

# Wifi
PRODUCT_COPY_FILES += \
    $(COMMON_PATH)/configs/wpa_supplicant_overlay.conf:system/vendor/etc/wifi/wpa_supplicant_overlay.conf \
    $(COMMON_PATH)/configs/p2p_supplicant_overlay.conf:system/vendor/etc/wifi/p2p_supplicant_overlay.conf \
    $(COMMON_PATH)/configs/bcmdhd.cal:system/vendor/etc/wifi/bcmdhd.cal

PRODUCT_PROPERTY_OVERRIDES += \
    wifi.interface=wlan0 \
    wifi.supplicant_scan_interval=30 \
    net.tethering.noprovisioning=true

$(call inherit-product-if-exists, hardware/broadcom/wlan/bcmdhd/firmware/bcm4330/device-bcm.mk)

# GAPPS
PRODUCT_COPY_FILES += \
    $(COMMON_PATH)/configs/google.xml:$(TARGET_COPY_OUT_VENDOR)/etc/sysconfig/google.xml

# Gps
PRODUCT_COPY_FILES += \
    $(COMMON_PATH)/configs/sirfgps.conf:$(TARGET_COPY_OUT_VENDOR)/etc/sirfgps.conf \
    $(COMMON_PATH)/configs/gps_debug.conf:$(TARGET_COPY_OUT_SYSTEM)/etc/gps_debug.conf \
    $(COMMON_PATH)/configs/gps.conf:$(TARGET_COPY_OUT_SYSTEM)/etc/gps.conf \

# Packages
PRODUCT_PACKAGES := \
    com.android.future.usb.accessory

# Permissions uvents
PRODUCT_PACKAGES += \
    ueventd.smdk4210.rc

# USB
PRODUCT_PACKAGES += \
    android.hardware.usb@1.3-service.basic

# Keymaster
PRODUCT_PACKAGES += \
    android.hardware.keymaster@3.0-impl \
    android.hardware.keymaster@3.0-service

# Touch features
PRODUCT_PACKAGES += \
    vendor.lineage.touch-service.samsung

# Legacy RIL
PRODUCT_PACKAGES += \
    libsecril-client \
    libsamsung_symbols \
    libsecril-shim \
    libcutils-s2-shim

# VNDK: libsec-ril.so is patched to load libcutils-v29.so for the
# record_stream functions, which the platform libcutils no longer has.
PRODUCT_COPY_FILES += \
    prebuilts/vndk/v34/arm/arch-arm-armv7-a-neon/shared/vndk-sp/libcutils.so:$(TARGET_COPY_OUT_VENDOR)/lib/libcutils-v29.so

# Legacy GPS
PRODUCT_PACKAGES += \
    android.hardware.gnss@1.0-impl \
    android.hardware.gnss@1.0-service.exynos4 \
    gps.smdk4210

# Power HAL
PRODUCT_PACKAGES += \
    android.hardware.power@1.0-impl \
    android.hardware.power@1.0-service

# Battery
PRODUCT_PACKAGES += \
     android.hardware.health@2.1-service \
     android.hardware.health@2.1-impl

PRODUCT_PROPERTY_OVERRIDES += \
    ro.config.small_battery=true

# Charger
PRODUCT_PROPERTY_OVERRIDES += \
    ro.charger.enable_suspend=true

# Use legacy ADB USB support
PRODUCT_PROPERTY_OVERRIDES += \
    ro.adb.nonblocking_ffs=false

# Don't swapoff zram on shutdown, it takes minutes on this kernel
PRODUCT_PROPERTY_OVERRIDES += \
    ro.init.shutdown.skip_zram_swapoff=true

# Audio Packages
PRODUCT_PACKAGES += \
    android.hardware.audio@7.0-impl \
    android.hardware.audio.effect@7.0-impl \
    android.hardware.audio.service.samsung.legacy \
    audio.primary.exynos4 \
    audio.r_submix.default \
    audio.usb.default \
    libaudiohw_legacy

# HAL
PRODUCT_PACKAGES += \
    vendor.lineage.livedisplay-service.samsung-exynos \
    android.hardware.graphics.allocator@2.0-impl-exynos4 \
    android.hardware.graphics.mapper@2.0-impl-exynos4 \
    android.hardware.graphics.composer@2.1-impl \
    android.hardware.keymaster@3.0-impl \
    gralloc.exynos4 \
    hwcomposer.exynos4 \
    libhwconverter \
    libs5pjpeg \
    libfimg \
    libsecion \
    libC

# Bluetooth
PRODUCT_PACKAGES += \
    audio.bluetooth.default \
    android.hardware.bluetooth.audio@2.0-impl \
    android.hardware.bluetooth@1.0-impl \
    android.hardware.bluetooth@1.0-service \
    libbt-vendor

# Gatekeeper
PRODUCT_PACKAGES += \
    android.hardware.gatekeeper@1.0-service.software

#Vibrator
PRODUCT_PACKAGES += \
    android.hardware.vibrator-service.legacy

# Camera (HAL v1 through the legacy 2.4 provider)
PRODUCT_PACKAGES += \
    android.hardware.camera.provider@2.4-impl \
    android.hardware.camera.provider@2.4-service \
    camera.device@1.0-impl \
    camera.smdk4210

# Old-API camera app (zero-copy HAL1 recording, 1080p), replaces Aperture
PRODUCT_PACKAGES += \
    OpenCamera

# Video recording still needs these, which predate Android 16
#PRODUCT_PACKAGES += \
#    libstagefright-shim \
#    mediaserver.rc

PRODUCT_PROPERTY_OVERRIDES += \
    media.settings.xml=/vendor/etc/media_profiles.xml \
    media.stagefright.legacyencoder=true \
    media.stagefright.less-secure=true \
    debug.stagefright.omx_default_rank.sw-audio=1 \
    debug.stagefright.omx_default_rank=0 \
    vendor.mediacodec.binder.size=4 \
    media.stagefright.thumbnail.prefer_hw_codecs=true \
    ro.camera.enableLazyHal=true

# MFC API
PRODUCT_PACKAGES += \
    libsecmfcapi

# Legacy OMX
PRODUCT_PROPERTY_OVERRIDES += \
    persist.media.treble_omx=false

#DRM
PRODUCT_PACKAGES += \
    android.hardware.drm@1.0-impl \
    android.hardware.drm@1.0-service-lazy

# Light
PRODUCT_PACKAGES += \
    android.hardware.light-service.samsung

# OMX
PRODUCT_PACKAGES += \
    libstagefrighthw \
    libseccscapi \
    libsecbasecomponent \
    libsecosal \
    libSEC_OMX_Resourcemanager \
    libSEC_OMX_Core \
    libSEC_OMX_Vdec \
    libOMX.SEC.AVC.Decoder \
    libOMX.SEC.M4V.Decoder \
    libOMX.SEC.WMV.Decoder \
    libOMX.SEC.AVC.Encoder \
    libSEC_OMX_Venc \
    libOMX.SEC.M4V.Encoder

PRODUCT_COPY_FILES += \
    $(COMMON_PATH)/configs/media_codecs.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs.xml \
    $(COMMON_PATH)/configs/media_profiles.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_profiles.xml \
    $(COMMON_PATH)/configs/media_profiles.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_profiles_V1_0.xml

# Graphics
PRODUCT_PROPERTY_OVERRIDES += \
    ro.opengles.version=131072 \
    ro.zygote.disable_gl_preload=true \
    ro.bq.gpu_to_cpu_unsupported=1 \
    debug.hwui.render_dirty_regions=false

# RIL
PRODUCT_PACKAGES += \
    android.hardware.radio.config@1.0-service

PRODUCT_PROPERTY_OVERRIDES += \
    ro.telephony.ril_class=SamsungExynos4RIL \
    mobiledata.interfaces=pdp0,gprs,ppp0,rmnet0,rmnet1 \
    ro.telephony.call_ring.multiple=false \
    ro.telephony.call_ring.delay=3000

# Memory Optimizations
# The kernel has no PSI, so the in-kernel lowmemorykiller does the killing
# and lmkd only passes it the minfree levels from ActivityManager. The
# ro.lmk.* tunables of the userspace killer have no effect here.
PRODUCT_PROPERTY_OVERRIDES += \
    ro.config.max_starting_bg=1 \
    ro.vendor.qti.am.reschedule_service=true \
    ro.vendor.qti.sys.fw.use_trim_settings=true \
    ro.vendor.qti.sys.fw.trim_empty_percent=50 \
    ro.vendor.qti.sys.fw.trim_cache_percent=100 \
    ro.vendor.qti.sys.fw.empty_app_percent=25

# Reduces GC frequency of foreground apps by 50%
PRODUCT_PROPERTY_OVERRIDES += \
    dalvik.vm.foreground-heap-growth-multiplier=2.0

# Services
PRODUCT_PROPERTY_OVERRIDES += \
    config.disable_atlas=true

# F2FS Filesystem
PRODUCT_PACKAGES += \
    fsck.f2fs

# NTFS Filesystem
PRODUCT_PACKAGES += \
    fsck.ntfs \
    mkfs.ntfs \
    mount.ntfs

# Exfat Filesystem
PRODUCT_PACKAGES += \
    fsck.exfat \
    mkfs.exfat
# Live Wallpapers
PRODUCT_PACKAGES += \
    LiveWallpapersPicker \
    librs_jni

# Wifi
PRODUCT_PACKAGES += \
    wificond \
    hostapd \
    libwpa_client \
    macloader \
    WifiOverlayGalaxyS2 \
    wpa_supplicant \
    wpa_supplicant.conf

# These are the hardware-specific features
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.camera.autofocus.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.camera.autofocus.xml \
    frameworks/native/data/etc/android.hardware.camera.flash-autofocus.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.camera.flash-autofocus.xml \
    frameworks/native/data/etc/android.hardware.camera.front.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.camera.front.xml \
    frameworks/native/data/etc/android.hardware.camera.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.camera.xml \
    frameworks/native/data/etc/android.hardware.location.gps.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.location.gps.xml \
    frameworks/native/data/etc/android.hardware.location.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.location.xml \
    frameworks/native/data/etc/android.hardware.sensor.accelerometer.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.sensor.accelerometer.xml \
    frameworks/native/data/etc/android.hardware.sensor.compass.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.sensor.compass.xml \
    frameworks/native/data/etc/android.hardware.sensor.gyroscope.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.sensor.gyroscope.xml \
    frameworks/native/data/etc/android.hardware.sensor.light.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.sensor.light.xml \
    frameworks/native/data/etc/android.hardware.sensor.proximity.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.sensor.proximity.xml \
    frameworks/native/data/etc/android.hardware.telephony.gsm.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.telephony.gsm.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.touchscreen.multitouch.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.touchscreen.xml \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/native/data/etc/android.hardware.usb.host.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.usb.host.xml \
    frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.xml \
    frameworks/native/data/etc/android.software.sip.voip.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.sip.voip.xml \
    frameworks/native/data/etc/android.software.sip.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.sip.xml \
    frameworks/native/data/etc/handheld_core_hardware.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/handheld_core_hardware.xml \
    frameworks/native/data/etc/android.hardware.bluetooth_le.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.bluetooth_le.xml

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/permissions/privapp-permissions-cellbroadcastappplatform.xml:$(TARGET_COPY_OUT_SYSTEM_EXT)/etc/permissions/privapp-permissions-cellbroadcastappplatform.xml \
    $(LOCAL_PATH)/configs/permissions/privapp-permissions-cellbroadcastservice.xml:$(TARGET_COPY_OUT_SYSTEM)/etc/permissions/privapp-permissions-cellbroadcastservice.xml

PRODUCT_TAGS += dalvik.gc.type-precise

# Keylayouts
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/excluded-input-devices.xml:system/etc/excluded-input-devices.xml

# Seccomp
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/seccomp/mediacodec.policy:$(TARGET_COPY_OUT_VENDOR)/etc/seccomp_policy/mediacodec.policy

# Include exynos4 platform specific parts
TARGET_HAL_PATH := hardware/samsung/exynos4/hal
TARGET_OMX_PATH := hardware/samsung/exynos/multimedia/openmax
$(call inherit-product, hardware/samsung/exynos4210.mk)

# Shipping API level
$(call inherit-product, $(SRC_TARGET_DIR)/product/product_launched_with_k.mk)

# Include non-open-source parts
$(call inherit-product, vendor/samsung/galaxys2-common/common-vendor.mk)

# Use GO
$(call inherit-product, build/make/target/product/go_defaults.mk)

# Apply Dalvik config for 1G phone
$(call inherit-product, frameworks/native/build/phone-xhdpi-1024-dalvik-heap.mk)

# Linux 3.0 has no userfaultfd, so ART falls back to the CC collector.
# Compile the boot image for it, with read barriers
PRODUCT_ENABLE_UFFD_GC := false

# Enable ADB debugging at boot
ifeq ($(WITH_ADB_DEBUG_AT_BOOT),true)
PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
    ro.adb.secure=0 \
    ro.debuggable=1 \
    persist.service.adb.enable=1 \
    persist.sys.usb.config=adb
endif
