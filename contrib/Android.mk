#
# Example Android.mk makefile for building examples for Android
# Thomas Perl <m@thp.io>, 2012-02-14
#
# Make sure to have a rooted Android device with a kernel that has the
# "hidraw" module built-in, or build your own kernel with CONFIG_HIDRAW
# enabled. You might also need the usual Bluez bluetoothd state file hacks.
#
# Use with agcc, e.g. from here:
# http://inportb.com/2012/01/08/agcc-bash-make-native-android-c-programs-using-the-ndk/
#

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := example
LOCAL_CFLAGS    :=
LOCAL_SRC_FILES := ../external/hidapi/linux/hid.c \
    ../src/psmove.c \
    ../examples/c/example.c
LOCAL_LDLIBS    :=

include $(BUILD_SHARED_LIBRARY)
