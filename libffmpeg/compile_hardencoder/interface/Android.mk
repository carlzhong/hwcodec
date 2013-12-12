LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../libffmpeg
LOCAL_CFLAGS := -fexceptions
LOCAL_SRC_FILES := interface.cpp CThread.cpp RealTimeEnc.cpp
LOCAL_LDLIBS := -llog  -landroid
LOCAL_LDLIBS += -ljnigraphics
LOCAL_SHARED_LIBRARIES += libffmpeg_neon   #libqcomomxsample_ics
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := videoeditor_neon


include $(BUILD_SHARED_LIBRARY)
