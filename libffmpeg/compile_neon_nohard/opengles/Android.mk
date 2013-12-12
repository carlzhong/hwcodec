# Copyright (C) 2009 The Android Open Source Project
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
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

ANDROID_SOURCE=/D/sourceCode/android4.0

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../libffmpeg \
        $(LOCAL_PATH)/../interface
	
LOCAL_C_INCLUDES += \
	$(ANDROID_SOURCE)/frameworks/base/core/jni/android/graphics \
	$(ANDROID_SOURCE)/frameworks/base/include	\
	$(ANDROID_SOURCE)/frameworks/base/native/include \
    $(ANDROID_SOURCE)/frameworks/base/opengl/include \
	$(ANDROID_SOURCE)/hardware/libhardware/include \
   	$(ANDROID_SOURCE)/system/core/include \
   	
LOCAL_MODULE    := gles2jni
LOCAL_CFLAGS    := -Werror
LOCAL_SRC_FILES := GLCode.cpp RenderIO.cpp ShaderUtil.cpp GLUtil.cpp


LOCAL_LDLIBS    := -llog -lGLESv2 -lGLESv1_CM  -L$(LOCAL_PATH)/../../obj/local/armeabi  -lGLESv2 -lvideoeditor_neon

LOCAL_LDLIBS    += -L$(ANDROID_SOURCE)/lib -lEGL -landroid  -lui   -ljnigraphics 
        
include $(BUILD_SHARED_LIBRARY)

#

include $(CLEAR_VARS)

ANDROID_SOURCE=/D/sourceCode/android4.0

LOCAL_C_INCLUDES += \
	$(ANDROID_SOURCE)/frameworks/base/core/jni/android/graphics \
	$(ANDROID_SOURCE)/frameworks/base/include	\
	$(ANDROID_SOURCE)/frameworks/base/native/include \
    $(ANDROID_SOURCE)/frameworks/base/opengl/include \
	$(ANDROID_SOURCE)/hardware/libhardware/include \
   	$(ANDROID_SOURCE)/system/core/include \

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../libffmpeg \
        $(LOCAL_PATH)/../interface

LOCAL_MODULE    := gles2jni_armv5te

LOCAL_CFLAGS    := -Werror 

LOCAL_SRC_FILES := GLCode.cpp RenderIO.cpp ShaderUtil.cpp GLUtil.cpp

LOCAL_LDLIBS    := -llog -lGLESv2 -lGLESv1_CM  -L$(LOCAL_PATH)/../../obj/local/armeabi  -lGLESv2 -lvideoeditor_armv5te

LOCAL_LDLIBS    += -L$(ANDROID_SOURCE)/lib -lEGL -landroid  -lui   -ljnigraphics

include $(BUILD_SHARED_LIBRARY)

#

include $(CLEAR_VARS)

ANDROID_SOURCE=/D/sourceCode/android4.0

LOCAL_C_INCLUDES += \
	$(ANDROID_SOURCE)/frameworks/base/core/jni/android/graphics \
	$(ANDROID_SOURCE)/frameworks/base/include	\
	$(ANDROID_SOURCE)/frameworks/base/native/include \
    $(ANDROID_SOURCE)/frameworks/base/opengl/include \
	$(ANDROID_SOURCE)/hardware/libhardware/include \
   	$(ANDROID_SOURCE)/system/core/include \
   	

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../libffmpeg \
        $(LOCAL_PATH)/../interface

LOCAL_MODULE    := gles2jni_vfp

LOCAL_CFLAGS    := -Werror 

LOCAL_SRC_FILES := GLCode.cpp RenderIO.cpp ShaderUtil.cpp GLUtil.cpp

LOCAL_LDLIBS    := -llog -lGLESv2 -lGLESv1_CM  -L$(LOCAL_PATH)/../../obj/local/armeabi  -lGLESv2 -lvideoeditor_vfp

LOCAL_LDLIBS    += -L$(ANDROID_SOURCE)/lib -lEGL -landroid  -lui    -ljnigraphics

include $(BUILD_SHARED_LIBRARY)

#

include $(CLEAR_VARS)

ANDROID_SOURCE=/D/sourceCode/android4.0

LOCAL_C_INCLUDES += \
	$(ANDROID_SOURCE)/frameworks/base/core/jni/android/graphics \
	$(ANDROID_SOURCE)/frameworks/base/include	\
	$(ANDROID_SOURCE)/frameworks/base/native/include \
    $(ANDROID_SOURCE)/frameworks/base/opengl/include \
	$(ANDROID_SOURCE)/hardware/libhardware/include \
   	$(ANDROID_SOURCE)/system/core/include \
   	
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../libffmpeg \
        $(LOCAL_PATH)/../interface

LOCAL_MODULE    := gles2jni_neon_nohard

LOCAL_CFLAGS    := -Werror 

LOCAL_SRC_FILES := GLCode.cpp RenderIO.cpp ShaderUtil.cpp GLUtil.cpp

LOCAL_LDLIBS    := -llog -lGLESv2 -lGLESv1_CM  -L$(LOCAL_PATH)/../../obj/local/armeabi  -lGLESv2 -lvideoeditor_neon_nohard

LOCAL_LDLIBS    += -L$(ANDROID_SOURCE)/lib -lEGL -landroid  -lui    -ljnigraphics

include $(BUILD_SHARED_LIBRARY)



