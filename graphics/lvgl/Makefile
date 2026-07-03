############################################################################
# apps/graphics/lvgl/Makefile
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

include $(APPDIR)/Make.defs

# LVGL graphic library

LVGL_DIR = .
LVGL_DIR_NAME = lvgl

-include ./lvgl/lvgl.mk

define LVGL_FILTER_OUT_DIR
CSRCS := $(filter-out $(LVGL_PATH)/$(1)/%,$(CSRCS))
CXXSRCS := $(filter-out $(LVGL_PATH)/$(1)/%,$(CXXSRCS))
ASRCS := $(filter-out $(LVGL_PATH)/$(1)/%,$(ASRCS))
endef

define LVGL_FILTER_OUT_FILE
CSRCS := $(filter-out $(LVGL_PATH)/$(1),$(CSRCS))
CXXSRCS := $(filter-out $(LVGL_PATH)/$(1),$(CXXSRCS))
ASRCS := $(filter-out $(LVGL_PATH)/$(1),$(ASRCS))
endef

ifeq ($(CONFIG_LV_BUILD_EXAMPLES),)
$(eval $(call LVGL_FILTER_OUT_DIR,examples))
endif

LVGL_ENABLED_DEMOS := \
  $(CONFIG_LV_USE_DEMO_WIDGETS) \
  $(CONFIG_LV_USE_DEMO_KEYPAD_AND_ENCODER) \
  $(CONFIG_LV_USE_DEMO_BENCHMARK) \
  $(CONFIG_LV_USE_DEMO_RENDER) \
  $(CONFIG_LV_USE_DEMO_SCROLL) \
  $(CONFIG_LV_USE_DEMO_STRESS) \
  $(CONFIG_LV_USE_DEMO_TRANSFORM) \
  $(CONFIG_LV_USE_DEMO_MUSIC) \
  $(CONFIG_LV_USE_DEMO_FLEX_LAYOUT) \
  $(CONFIG_LV_USE_DEMO_MULTILANG) \
  $(CONFIG_LV_USE_DEMO_VECTOR_GRAPHIC)

ifeq ($(filter y,$(LVGL_ENABLED_DEMOS)),)
$(eval $(call LVGL_FILTER_OUT_DIR,demos))
endif

ifeq ($(CONFIG_LV_USE_DRAW_SW),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/draw/sw))
endif

ifeq ($(CONFIG_LV_DRAW_SW_ASM_HELIUM),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/draw/sw/blend/helium))
endif

ifeq ($(CONFIG_LV_DRAW_SW_ASM_NEON),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/draw/sw/blend/neon))
endif

ifeq ($(CONFIG_LV_USE_DRAW_PXP),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/draw/nxp/pxp))
endif

ifeq ($(CONFIG_LV_USE_DRAW_VGLITE),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/draw/nxp/vglite))
endif

ifeq ($(CONFIG_LV_USE_DRAW_DAVE2D),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/draw/renesas/dave2d))
endif

ifeq ($(CONFIG_LV_USE_DRAW_SDL),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/draw/sdl))
endif

ifeq ($(CONFIG_LV_USE_DRAW_VG_LITE),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/draw/vg_lite))
endif

ifeq ($(CONFIG_LV_USE_LINUX_DRM),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/drivers/display/drm))
endif

ifeq ($(CONFIG_LV_USE_LINUX_FBDEV),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/drivers/display/fb))
endif

ifeq ($(CONFIG_LV_USE_ILI9341),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/drivers/display/ili9341))
endif

ifeq ($(CONFIG_LV_USE_GENERIC_MIPI),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/drivers/display/lcd))
endif

ifeq ($(CONFIG_LV_USE_RENESAS_GLCDC),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/drivers/display/renesas_glcdc))
endif

ifeq ($(CONFIG_LV_USE_ST7735),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/drivers/display/st7735))
endif

ifeq ($(CONFIG_LV_USE_ST7789),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/drivers/display/st7789))
endif

ifeq ($(CONFIG_LV_USE_ST7796),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/drivers/display/st7796))
endif

ifeq ($(CONFIG_LV_USE_TFT_ESPI),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/drivers/display/tft_espi))
endif

ifeq ($(CONFIG_LV_USE_EVDEV),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/drivers/evdev))
endif

ifeq ($(CONFIG_LV_USE_OPENGLES),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/drivers/glfw))
endif

ifeq ($(CONFIG_LV_USE_LIBINPUT),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/drivers/libinput))
endif

ifeq ($(CONFIG_LV_USE_NUTTX),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/drivers/nuttx))
else
ifeq ($(CONFIG_LV_USE_NUTTX_LCD),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/drivers/nuttx/lv_nuttx_lcd.c))
endif
ifeq ($(CONFIG_LV_USE_NUTTX_LIBUV),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/drivers/nuttx/lv_nuttx_libuv.c))
endif
ifeq ($(CONFIG_LV_USE_NUTTX_TOUCHSCREEN),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/drivers/nuttx/lv_nuttx_touchscreen.c))
endif
endif

ifeq ($(CONFIG_LV_USE_QNX),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/drivers/qnx))
endif

ifeq ($(CONFIG_LV_USE_SDL),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/drivers/sdl))
endif

ifeq ($(CONFIG_LV_USE_WAYLAND),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/drivers/wayland))
endif

ifeq ($(CONFIG_LV_OS_WINDOWS),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/drivers/windows))
endif

ifeq ($(CONFIG_LV_USE_X11),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/drivers/x11))
endif

ifeq ($(CONFIG_LV_FONT_MONTSERRAT_8),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/font/lv_font_montserrat_8.c))
endif
ifeq ($(CONFIG_LV_FONT_MONTSERRAT_10),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/font/lv_font_montserrat_10.c))
endif
ifeq ($(CONFIG_LV_FONT_MONTSERRAT_12),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/font/lv_font_montserrat_12.c))
endif
ifeq ($(CONFIG_LV_FONT_MONTSERRAT_14),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/font/lv_font_montserrat_14.c))
endif
ifeq ($(CONFIG_LV_FONT_MONTSERRAT_16),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/font/lv_font_montserrat_16.c))
endif
ifeq ($(CONFIG_LV_FONT_MONTSERRAT_18),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/font/lv_font_montserrat_18.c))
endif
ifeq ($(CONFIG_LV_FONT_MONTSERRAT_20),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/font/lv_font_montserrat_20.c))
endif
ifeq ($(CONFIG_LV_FONT_MONTSERRAT_22),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/font/lv_font_montserrat_22.c))
endif
ifeq ($(CONFIG_LV_FONT_MONTSERRAT_24),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/font/lv_font_montserrat_24.c))
endif
ifeq ($(CONFIG_LV_FONT_MONTSERRAT_26),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/font/lv_font_montserrat_26.c))
endif
ifeq ($(CONFIG_LV_FONT_MONTSERRAT_28),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/font/lv_font_montserrat_28.c))
endif
ifeq ($(CONFIG_LV_FONT_MONTSERRAT_30),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/font/lv_font_montserrat_30.c))
endif
ifeq ($(CONFIG_LV_FONT_MONTSERRAT_32),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/font/lv_font_montserrat_32.c))
endif
ifeq ($(CONFIG_LV_FONT_MONTSERRAT_34),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/font/lv_font_montserrat_34.c))
endif
ifeq ($(CONFIG_LV_FONT_MONTSERRAT_36),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/font/lv_font_montserrat_36.c))
endif
ifeq ($(CONFIG_LV_FONT_MONTSERRAT_38),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/font/lv_font_montserrat_38.c))
endif
ifeq ($(CONFIG_LV_FONT_MONTSERRAT_40),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/font/lv_font_montserrat_40.c))
endif
ifeq ($(CONFIG_LV_FONT_MONTSERRAT_42),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/font/lv_font_montserrat_42.c))
endif
ifeq ($(CONFIG_LV_FONT_MONTSERRAT_44),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/font/lv_font_montserrat_44.c))
endif
ifeq ($(CONFIG_LV_FONT_MONTSERRAT_46),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/font/lv_font_montserrat_46.c))
endif
ifeq ($(CONFIG_LV_FONT_MONTSERRAT_48),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/font/lv_font_montserrat_48.c))
endif
ifeq ($(CONFIG_LV_FONT_MONTSERRAT_28_COMPRESSED),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/font/lv_font_montserrat_28_compressed.c))
endif
ifeq ($(CONFIG_LV_FONT_DEJAVU_16_PERSIAN_HEBREW),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/font/lv_font_dejavu_16_persian_hebrew.c))
endif
ifeq ($(CONFIG_LV_FONT_SIMSUN_14_CJK),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/font/lv_font_simsun_14_cjk.c))
endif
ifeq ($(CONFIG_LV_FONT_SIMSUN_16_CJK),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/font/lv_font_simsun_16_cjk.c))
endif
ifeq ($(CONFIG_LV_FONT_UNSCII_8),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/font/lv_font_unscii_8.c))
endif
ifeq ($(CONFIG_LV_FONT_UNSCII_16),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/font/lv_font_unscii_16.c))
endif

ifeq ($(CONFIG_LV_USE_FS_STDIO),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/libs/fsdrv/lv_fs_stdio.c))
endif
ifeq ($(CONFIG_LV_USE_FS_POSIX),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/libs/fsdrv/lv_fs_posix.c))
endif
ifeq ($(CONFIG_LV_USE_FS_WIN32),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/libs/fsdrv/lv_fs_win32.c))
endif
ifeq ($(CONFIG_LV_USE_FS_FATFS),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/libs/fsdrv/lv_fs_fatfs.c))
endif
ifeq ($(CONFIG_LV_USE_FS_MEMFS),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/libs/fsdrv/lv_fs_memfs.c))
endif
ifeq ($(CONFIG_LV_USE_FS_LITTLEFS),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/libs/fsdrv/lv_fs_littlefs.c))
endif
ifeq ($(CONFIG_LV_USE_FS_ARDUINO_ESP_LITTLEFS),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/libs/fsdrv/lv_fs_arduino_esp_littlefs.cpp))
endif
ifeq ($(CONFIG_LV_USE_FS_ARDUINO_SD),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/libs/fsdrv/lv_fs_arduino_sd.cpp))
endif

ifeq ($(CONFIG_LV_USE_BARCODE),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/libs/barcode))
endif
ifeq ($(CONFIG_LV_USE_BMP),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/libs/bmp))
endif
ifeq ($(CONFIG_LV_USE_FFMPEG),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/libs/ffmpeg))
endif
ifeq ($(CONFIG_LV_USE_FREETYPE),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/libs/freetype))
endif
ifeq ($(CONFIG_LV_USE_GIF),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/libs/gif))
endif
ifeq ($(CONFIG_LV_USE_LIBJPEG_TURBO),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/libs/libjpeg_turbo))
endif
ifeq ($(CONFIG_LV_USE_LIBPNG),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/libs/libpng))
endif
ifeq ($(CONFIG_LV_USE_LODEPNG),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/libs/lodepng))
endif
ifeq ($(CONFIG_LV_USE_LZ4),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/libs/lz4))
endif
ifeq ($(CONFIG_LV_USE_QRCODE),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/libs/qrcode))
endif
ifeq ($(CONFIG_LV_USE_RLE),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/libs/rle))
endif
ifeq ($(CONFIG_LV_USE_RLOTTIE),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/libs/rlottie))
endif
ifeq ($(CONFIG_LV_USE_THORVG),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/libs/thorvg))
endif
ifeq ($(CONFIG_LV_USE_TINY_TTF),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/libs/tiny_ttf))
endif
ifeq ($(CONFIG_LV_USE_TJPGD),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/libs/tjpgd))
endif

ifeq ($(CONFIG_LV_OS_CMSIS_RTOS2),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/osal/lv_cmsis_rtos2.c))
endif
ifeq ($(CONFIG_LV_OS_FREERTOS),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/osal/lv_freertos.c))
endif
ifeq ($(CONFIG_LV_OS_MQX),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/osal/lv_mqx.c))
endif
ifeq ($(CONFIG_LV_OS_NONE),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/osal/lv_os_none.c))
endif
ifeq ($(CONFIG_LV_OS_PTHREAD),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/osal/lv_pthread.c))
endif
ifeq ($(CONFIG_LV_OS_RTTHREAD),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/osal/lv_rtthread.c))
endif
ifeq ($(CONFIG_LV_OS_WINDOWS),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/osal/lv_windows.c))
endif

ifeq ($(filter y,$(CONFIG_LV_USE_BUILTIN_MALLOC) $(CONFIG_LV_USE_BUILTIN_STRING) $(CONFIG_LV_USE_BUILTIN_SPRINTF)),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/stdlib/builtin))
endif
ifeq ($(filter y,$(CONFIG_LV_USE_CLIB_MALLOC) $(CONFIG_LV_USE_CLIB_STRING) $(CONFIG_LV_USE_CLIB_SPRINTF)),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/stdlib/clib))
endif
ifeq ($(CONFIG_LV_USE_MICROPYTHON_MALLOC),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/stdlib/micropython))
endif
ifeq ($(CONFIG_LV_USE_RTTHREAD_MALLOC),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/stdlib/rtthread))
endif

ifeq ($(CONFIG_LV_USE_FILE_EXPLORER),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/others/file_explorer))
endif
ifeq ($(CONFIG_LV_USE_FRAGMENT),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/others/fragment))
endif
ifeq ($(CONFIG_LV_USE_GRIDNAV),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/others/gridnav))
endif
ifeq ($(CONFIG_LV_USE_IME_PINYIN),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/others/ime))
endif
ifeq ($(CONFIG_LV_USE_IMGFONT),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/others/imgfont))
endif
ifeq ($(CONFIG_LV_USE_MONKEY),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/others/monkey))
endif
ifeq ($(CONFIG_LV_USE_OBSERVER),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/others/observer))
endif
ifeq ($(CONFIG_LV_USE_SNAPSHOT),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/others/snapshot))
endif
ifeq ($(CONFIG_LV_USE_SYSMON),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/others/sysmon))
endif
ifeq ($(CONFIG_LV_USE_VG_LITE_THORVG),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/others/vg_lite_tvg))
endif

ifeq ($(CONFIG_LV_USE_THEME_DEFAULT),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/themes/default))
endif
ifeq ($(CONFIG_LV_USE_THEME_MONO),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/themes/mono))
endif
ifeq ($(CONFIG_LV_USE_THEME_SIMPLE),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/themes/simple))
endif

ifeq ($(CONFIG_LV_CALENDAR_CHINESE),)
$(eval $(call LVGL_FILTER_OUT_FILE,src/widgets/calendar/lv_calendar_chinese.c))
endif
ifeq ($(CONFIG_LV_USE_LOTTIE),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/widgets/lottie))
endif
ifeq ($(CONFIG_LV_USE_OBJ_PROPERTY),)
$(eval $(call LVGL_FILTER_OUT_DIR,src/widgets/property))
endif

ifneq ($(CONFIG_LV_ASSERT_HANDLER_INCLUDE), "")
CFLAGS += "-DLV_ASSERT_HANDLER=ASSERT(0);"
endif

ifeq ($(and $(CONFIG_LV_USE_PERF_MONITOR),$(CONFIG_SCHED_CPULOAD)),y)
CFLAGS += "-DLV_SYSMON_GET_IDLE=lv_nuttx_get_idle"
endif

ifeq ($(and $(CONFIG_SCHED_INSTRUMENTATION),$(CONFIG_LV_USE_PROFILER)),y)
CFLAGS += "-DLV_PROFILER_BEGIN=sched_note_beginex(NOTE_TAG_GRAPHICS, __func__)"
CFLAGS += "-DLV_PROFILER_END=sched_note_endex(NOTE_TAG_GRAPHICS, __func__)"

CFLAGS += "-DLV_PROFILER_BEGIN_TAG(str)=sched_note_beginex(NOTE_TAG_GRAPHICS, str)"
CFLAGS += "-DLV_PROFILER_END_TAG(str)=sched_note_endex(NOTE_TAG_GRAPHICS, str)"
endif

ifneq ($(CONFIG_LV_OPTLEVEL), "")
# Since multiple options need to be supported, subst needs to be used here to remove
# the redundant double quotes, otherwise it will cause command parsing errors.
CFLAGS   += $(subst ",, $(CONFIG_LV_OPTLEVEL))
CXXFLAGS += $(subst ",, $(CONFIG_LV_OPTLEVEL))
endif

# Set up build configuration and environment

WD := ${shell echo $(CURDIR) | sed -e 's/ /\\ /g'}

CONFIG_GRAPH_LVGL_URL ?= "https://github.com/lvgl/lvgl/archive/refs/tags"

_MAJ         := $(CONFIG_LVGL_VERSION_MAJOR)
_MIN         := $(CONFIG_LVGL_VERSION_MINOR)
_PAT         := $(CONFIG_LVGL_VERSION_PATCH)
LVGL_VERSION := $(_MAJ).$(_MIN).$(_PAT)
LVGL_TARBALL := v$(LVGL_VERSION).zip

LVGL_UNPACKNAME = lvgl
UNPACK ?= unzip -o $(if $(V),,-q)
CURL ?= curl -L -O $(if $(V),,-Ss)

LVGL_UNPACKDIR =  $(WD)/$(LVGL_UNPACKNAME)

$(LVGL_TARBALL):
	$(ECHO_BEGIN)"Downloading: $(LVGL_TARBALL)"
	$(Q) $(CURL) $(CONFIG_GRAPH_LVGL_URL)/$(LVGL_TARBALL)
	$(ECHO_END)

$(LVGL_UNPACKNAME): $(LVGL_TARBALL)
	$(ECHO_BEGIN)"Unpacking: $(LVGL_TARBALL) -> $(LVGL_UNPACKNAME)"
	$(Q) $(UNPACK) $(LVGL_TARBALL)
	$(Q) mv	lvgl-$(LVGL_VERSION) $(LVGL_UNPACKNAME)
	$(Q) touch $(LVGL_UNPACKNAME)
	$(ECHO_END)

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(LVGL_UNPACKNAME)/.git),)
context:: $(LVGL_UNPACKNAME)
endif

include $(APPDIR)/Application.mk

ifeq ($(wildcard $(LVGL_UNPACKNAME)/.git),)
distclean::
	$(call DELDIR, $(LVGL_UNPACKNAME))
	$(call DELFILE, $(LVGL_TARBALL))
endif
