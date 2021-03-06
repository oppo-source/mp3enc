LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../../include/ \
	$(LOCAL_PATH)/../../include/

LOCAL_SHARED_LIBRARIES := \
        libbinder \
        libcutils \
        libdl \
        libdrmframework \
        libexpat \
        libgui \
        liblog \
        libui \
        libstagefright_foundation \
        libutils \
        libz \


LOCAL_SRC_FILES := libmp3lame/VbrTag.c \
	libmp3lame/bitstream.c \
	libmp3lame/encoder.c \
	libmp3lame/fft.c \
	libmp3lame/gain_analysis.c \
	libmp3lame/id3tag.c \
	libmp3lame/lame.c \
	libmp3lame/mpglib_interface.c \
	libmp3lame/newmdct.c \
	libmp3lame/presets.c \
	libmp3lame/psymodel.c \
	libmp3lame/quantize.c \
	libmp3lame/quantize_pvt.c \
	libmp3lame/reservoir.c \
	libmp3lame/set_get.c \
	libmp3lame/tables.c \
	libmp3lame/takehiro.c \
	libmp3lame/util.c \
	libmp3lame/vbrquantize.c \
	libmp3lame/version.c  \
	MP3Encoder.cpp  \
	../../avc_utils.cpp  \
	../../MediaBuffer.cpp  \
	../../MediaBufferGroup.cpp  \
	../../MediaSource.cpp  \
	../../MetaData.cpp

LOCAL_CFLAGS := -Wno-error=tautological-compare
LOCAL_MODULE := liboplusstagefright__mp3enc
LOCAL_SYSTEM_EXT_MODULE := true

#LOCAL_MULTILIB := 64

#include $(BUILD_STATIC_LIBRARY)
include $(BUILD_SHARED_LIBRARY)

