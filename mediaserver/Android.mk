# Overrides the stock mediaserver.rc to preload libstagefright-shim, which is
# only built along with the camera HAL1 stack
ifeq ($(TARGET_HAS_LEGACY_CAMERA_HAL1),true)
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := mediaserver.rc
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH  := $(TARGET_OUT)/etc/init
LOCAL_SRC_FILES := mediaserver.rc

include $(BUILD_PREBUILT)
endif
