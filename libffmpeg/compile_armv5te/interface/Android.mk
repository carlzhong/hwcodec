LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../libffmpeg/config.mak
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../libffmpeg
LOCAL_CFLAGS := -fexceptions
LOCAL_SRC_FILES := interface.cpp CThread.cpp RealTimeEnc.cpp
LOCAL_LDLIBS := -llog  -landroid
LOCAL_LDLIBS += -ljnigraphics

ifeq ($(ARCH_ARM),yes)
ifeq ($(HAVE_NEON),yes)
LOCAL_SHARED_LIBRARIES += libffmpeg_neon_nohard  #libqcomomxsample_ics
LOCAL_MODULE := videoeditor_neon_nohard
else
ifeq ($(HAVE_ARMVFP),yes)
LOCAL_SHARED_LIBRARIES += libffmpeg_vfp
LOCAL_MODULE := videoeditor_vfp
else
LOCAL_SHARED_LIBRARIES += libffmpeg_armv5te
LOCAL_MODULE := videoeditor_armv5te
endif
endif
endif


LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
