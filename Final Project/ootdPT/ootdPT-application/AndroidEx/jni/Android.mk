LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := dev_driver
LOCAL_SRC_FILES := dev_driver.c
LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := intr_driver
LOCAL_SRC_FILES := intr_driver.c
LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)
