LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../libffmpeg
LOCAL_CFLAGS := -fexceptions
LOCAL_SRC_FILES := interface.cpp CThread.cpp RealTimeEnc.cpp  AudioDec.cpp
LOCAL_LDLIBS := -llog  -landroid
LOCAL_LDLIBS += -ljnigraphics
LOCAL_SHARED_LIBRARIES += libhwffmpeg_neon  #libffmpeg_neon   #libqcomomxsample_ics
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE :=   hwvideoeditor_neon  #videoeditor_neon


include $(BUILD_SHARED_LIBRARY)
