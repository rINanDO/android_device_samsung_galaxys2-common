LOCAL_PATH := $(call my-dir)

# Packages inherited from AOSP/Lineage product configs that are useless or
# too heavy on this 1 GB device. Overriding them keeps them out of the image.
include $(CLEAR_VARS)
LOCAL_MODULE := RemovePackages
LOCAL_MODULE_CLASS := APPS
LOCAL_MODULE_TAGS := optional
LOCAL_OVERRIDES_PACKAGES := \
    AudioFX \
    DeviceAsWebcam \
    ImsServiceEntitlement \
    Seedvault
LOCAL_UNINSTALLABLE_MODULE := true
LOCAL_CERTIFICATE := PRESIGNED
LOCAL_SRC_FILES := /dev/null
include $(BUILD_PREBUILT)
