LIBHYBRIS_TOP := $(call my-dir)

# Build all compat layers explicitly
include $(LIBHYBRIS_TOP)/compat/ui/Android.mk
include $(LIBHYBRIS_TOP)/compat/input/Android.mk
include $(LIBHYBRIS_TOP)/compat/surface_flinger/Android.mk
include $(LIBHYBRIS_TOP)/compat/hwc2/Android.mk
include $(LIBHYBRIS_TOP)/compat/media/Android.mk
include $(LIBHYBRIS_TOP)/compat/camera/Android.mk
