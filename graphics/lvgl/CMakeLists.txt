# ##############################################################################
# apps/graphics/lvgl/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more contributor
# license agreements.  See the NOTICE file distributed with this work for
# additional information regarding copyright ownership.  The ASF licenses this
# file to you under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.  You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations under
# the License.
#
# ##############################################################################

if(CONFIG_GRAPHICS_LVGL)

  # ############################################################################
  # Config and Fetch lvgl
  # ############################################################################
  set(LVGL_DIR ${CMAKE_CURRENT_LIST_DIR}/lvgl)

  set(LVGL_VERSION
      "${CONFIG_LVGL_VERSION_MAJOR}.${CONFIG_LVGL_VERSION_MINOR}.${CONFIG_LVGL_VERSION_PATCH}"
  )

  if(NOT EXISTS ${LVGL_DIR})
    set(LVGL_URL
        "https://github.com/lvgl/lvgl/archive/refs/tags/v${LVGL_VERSION}.zip")
    FetchContent_Declare(
      lvgl_fetch
      URL ${LVGL_URL}
          DOWNLOAD_DIR
          ${CMAKE_CURRENT_LIST_DIR}
          SOURCE_DIR
          ${CMAKE_CURRENT_LIST_DIR}/lvgl
          BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/graphics/lvgl/lvgl
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 120)

    FetchContent_GetProperties(lvgl_fetch)

    if(NOT lvgl_fetch_POPULATED)
      FetchContent_Populate(lvgl_fetch)
    endif()
  endif()

  # ############################################################################
  # Flags and Sources
  # ############################################################################

  # ############################################################################
  # Library Configuration
  # ############################################################################

  file(
    GLOB_RECURSE
    SRCS
    "${LVGL_DIR}/src/*.S"
    "${LVGL_DIR}/src/*.c"
    "${LVGL_DIR}/src/*.cpp")

  function(lvgl_exclude_dir DIR)
    list(FILTER SRCS EXCLUDE REGEX "^${LVGL_DIR}/${DIR}/")
    set(SRCS "${SRCS}" PARENT_SCOPE)
  endfunction()

  function(lvgl_exclude_file FILE)
    list(FILTER SRCS EXCLUDE REGEX "^${LVGL_DIR}/${FILE}$")
    set(SRCS "${SRCS}" PARENT_SCOPE)
  endfunction()

  if(CONFIG_LV_BUILD_EXAMPLES)
    file(GLOB_RECURSE LVGL_EXAMPLE_SRCS "${LVGL_DIR}/examples/*.c")
    list(APPEND SRCS ${LVGL_EXAMPLE_SRCS})
  endif()

  if(CONFIG_LV_USE_DEMO_WIDGETS
     OR CONFIG_LV_USE_DEMO_KEYPAD_AND_ENCODER
     OR CONFIG_LV_USE_DEMO_BENCHMARK
     OR CONFIG_LV_USE_DEMO_RENDER
     OR CONFIG_LV_USE_DEMO_SCROLL
     OR CONFIG_LV_USE_DEMO_STRESS
     OR CONFIG_LV_USE_DEMO_TRANSFORM
     OR CONFIG_LV_USE_DEMO_MUSIC
     OR CONFIG_LV_USE_DEMO_FLEX_LAYOUT
     OR CONFIG_LV_USE_DEMO_MULTILANG
     OR CONFIG_LV_USE_DEMO_VECTOR_GRAPHIC)
    file(GLOB_RECURSE LVGL_DEMO_SRCS "${LVGL_DIR}/demos/*.c")
    list(APPEND SRCS ${LVGL_DEMO_SRCS})
  endif()

  if(NOT CONFIG_LV_USE_DRAW_SW)
    lvgl_exclude_dir("src/draw/sw")
  endif()
  if(NOT CONFIG_LV_DRAW_SW_ASM_HELIUM)
    lvgl_exclude_dir("src/draw/sw/blend/helium")
  endif()
  if(NOT CONFIG_LV_DRAW_SW_ASM_NEON)
    lvgl_exclude_dir("src/draw/sw/blend/neon")
  endif()
  if(NOT CONFIG_LV_USE_DRAW_PXP)
    lvgl_exclude_dir("src/draw/nxp/pxp")
  endif()
  if(NOT CONFIG_LV_USE_DRAW_VGLITE)
    lvgl_exclude_dir("src/draw/nxp/vglite")
  endif()
  if(NOT CONFIG_LV_USE_DRAW_DAVE2D)
    lvgl_exclude_dir("src/draw/renesas/dave2d")
  endif()
  if(NOT CONFIG_LV_USE_DRAW_SDL)
    lvgl_exclude_dir("src/draw/sdl")
  endif()
  if(NOT CONFIG_LV_USE_DRAW_VG_LITE)
    lvgl_exclude_dir("src/draw/vg_lite")
  endif()

  if(NOT CONFIG_LV_USE_LINUX_DRM)
    lvgl_exclude_dir("src/drivers/display/drm")
  endif()
  if(NOT CONFIG_LV_USE_LINUX_FBDEV)
    lvgl_exclude_dir("src/drivers/display/fb")
  endif()
  if(NOT CONFIG_LV_USE_ILI9341)
    lvgl_exclude_dir("src/drivers/display/ili9341")
  endif()
  if(NOT CONFIG_LV_USE_GENERIC_MIPI)
    lvgl_exclude_dir("src/drivers/display/lcd")
  endif()
  if(NOT CONFIG_LV_USE_RENESAS_GLCDC)
    lvgl_exclude_dir("src/drivers/display/renesas_glcdc")
  endif()
  if(NOT CONFIG_LV_USE_ST7735)
    lvgl_exclude_dir("src/drivers/display/st7735")
  endif()
  if(NOT CONFIG_LV_USE_ST7789)
    lvgl_exclude_dir("src/drivers/display/st7789")
  endif()
  if(NOT CONFIG_LV_USE_ST7796)
    lvgl_exclude_dir("src/drivers/display/st7796")
  endif()
  if(NOT CONFIG_LV_USE_TFT_ESPI)
    lvgl_exclude_dir("src/drivers/display/tft_espi")
  endif()
  if(NOT CONFIG_LV_USE_EVDEV)
    lvgl_exclude_dir("src/drivers/evdev")
  endif()
  if(NOT CONFIG_LV_USE_OPENGLES)
    lvgl_exclude_dir("src/drivers/glfw")
  endif()
  if(NOT CONFIG_LV_USE_LIBINPUT)
    lvgl_exclude_dir("src/drivers/libinput")
  endif()
  if(NOT CONFIG_LV_USE_NUTTX)
    lvgl_exclude_dir("src/drivers/nuttx")
  else()
    if(NOT CONFIG_LV_USE_NUTTX_LCD)
      lvgl_exclude_file("src/drivers/nuttx/lv_nuttx_lcd.c")
    endif()
    if(NOT CONFIG_LV_USE_NUTTX_LIBUV)
      lvgl_exclude_file("src/drivers/nuttx/lv_nuttx_libuv.c")
    endif()
    if(NOT CONFIG_LV_USE_NUTTX_TOUCHSCREEN)
      lvgl_exclude_file("src/drivers/nuttx/lv_nuttx_touchscreen.c")
    endif()
  endif()
  if(NOT CONFIG_LV_USE_QNX)
    lvgl_exclude_dir("src/drivers/qnx")
  endif()
  if(NOT CONFIG_LV_USE_SDL)
    lvgl_exclude_dir("src/drivers/sdl")
  endif()
  if(NOT CONFIG_LV_USE_WAYLAND)
    lvgl_exclude_dir("src/drivers/wayland")
  endif()
  if(NOT CONFIG_LV_OS_WINDOWS)
    lvgl_exclude_dir("src/drivers/windows")
  endif()
  if(NOT CONFIG_LV_USE_X11)
    lvgl_exclude_dir("src/drivers/x11")
  endif()

  foreach(FONT_SIZE 8 10 12 14 16 18 20 22 24 26 28 30 32 34 36 38 40 42 44 46 48)
    if(NOT CONFIG_LV_FONT_MONTSERRAT_${FONT_SIZE})
      lvgl_exclude_file("src/font/lv_font_montserrat_${FONT_SIZE}.c")
    endif()
  endforeach()
  if(NOT CONFIG_LV_FONT_MONTSERRAT_28_COMPRESSED)
    lvgl_exclude_file("src/font/lv_font_montserrat_28_compressed.c")
  endif()
  if(NOT CONFIG_LV_FONT_DEJAVU_16_PERSIAN_HEBREW)
    lvgl_exclude_file("src/font/lv_font_dejavu_16_persian_hebrew.c")
  endif()
  if(NOT CONFIG_LV_FONT_SIMSUN_14_CJK)
    lvgl_exclude_file("src/font/lv_font_simsun_14_cjk.c")
  endif()
  if(NOT CONFIG_LV_FONT_SIMSUN_16_CJK)
    lvgl_exclude_file("src/font/lv_font_simsun_16_cjk.c")
  endif()
  if(NOT CONFIG_LV_FONT_UNSCII_8)
    lvgl_exclude_file("src/font/lv_font_unscii_8.c")
  endif()
  if(NOT CONFIG_LV_FONT_UNSCII_16)
    lvgl_exclude_file("src/font/lv_font_unscii_16.c")
  endif()

  if(NOT CONFIG_LV_USE_FS_STDIO)
    lvgl_exclude_file("src/libs/fsdrv/lv_fs_stdio.c")
  endif()
  if(NOT CONFIG_LV_USE_FS_POSIX)
    lvgl_exclude_file("src/libs/fsdrv/lv_fs_posix.c")
  endif()
  if(NOT CONFIG_LV_USE_FS_WIN32)
    lvgl_exclude_file("src/libs/fsdrv/lv_fs_win32.c")
  endif()
  if(NOT CONFIG_LV_USE_FS_FATFS)
    lvgl_exclude_file("src/libs/fsdrv/lv_fs_fatfs.c")
  endif()
  if(NOT CONFIG_LV_USE_FS_MEMFS)
    lvgl_exclude_file("src/libs/fsdrv/lv_fs_memfs.c")
  endif()
  if(NOT CONFIG_LV_USE_FS_LITTLEFS)
    lvgl_exclude_file("src/libs/fsdrv/lv_fs_littlefs.c")
  endif()
  if(NOT CONFIG_LV_USE_FS_ARDUINO_ESP_LITTLEFS)
    lvgl_exclude_file("src/libs/fsdrv/lv_fs_arduino_esp_littlefs.cpp")
  endif()
  if(NOT CONFIG_LV_USE_FS_ARDUINO_SD)
    lvgl_exclude_file("src/libs/fsdrv/lv_fs_arduino_sd.cpp")
  endif()

  if(NOT CONFIG_LV_USE_BARCODE)
    lvgl_exclude_dir("src/libs/barcode")
  endif()
  if(NOT CONFIG_LV_USE_BMP)
    lvgl_exclude_dir("src/libs/bmp")
  endif()
  if(NOT CONFIG_LV_USE_FFMPEG)
    lvgl_exclude_dir("src/libs/ffmpeg")
  endif()
  if(NOT CONFIG_LV_USE_FREETYPE)
    lvgl_exclude_dir("src/libs/freetype")
  endif()
  if(NOT CONFIG_LV_USE_GIF)
    lvgl_exclude_dir("src/libs/gif")
  endif()
  if(NOT CONFIG_LV_USE_LIBJPEG_TURBO)
    lvgl_exclude_dir("src/libs/libjpeg_turbo")
  endif()
  if(NOT CONFIG_LV_USE_LIBPNG)
    lvgl_exclude_dir("src/libs/libpng")
  endif()
  if(NOT CONFIG_LV_USE_LODEPNG)
    lvgl_exclude_dir("src/libs/lodepng")
  endif()
  if(NOT CONFIG_LV_USE_LZ4)
    lvgl_exclude_dir("src/libs/lz4")
  endif()
  if(NOT CONFIG_LV_USE_QRCODE)
    lvgl_exclude_dir("src/libs/qrcode")
  endif()
  if(NOT CONFIG_LV_USE_RLE)
    lvgl_exclude_dir("src/libs/rle")
  endif()
  if(NOT CONFIG_LV_USE_RLOTTIE)
    lvgl_exclude_dir("src/libs/rlottie")
  endif()
  if(NOT CONFIG_LV_USE_THORVG)
    lvgl_exclude_dir("src/libs/thorvg")
  endif()
  if(NOT CONFIG_LV_USE_TINY_TTF)
    lvgl_exclude_dir("src/libs/tiny_ttf")
  endif()
  if(NOT CONFIG_LV_USE_TJPGD)
    lvgl_exclude_dir("src/libs/tjpgd")
  endif()

  if(NOT CONFIG_LV_OS_CMSIS_RTOS2)
    lvgl_exclude_file("src/osal/lv_cmsis_rtos2.c")
  endif()
  if(NOT CONFIG_LV_OS_FREERTOS)
    lvgl_exclude_file("src/osal/lv_freertos.c")
  endif()
  if(NOT CONFIG_LV_OS_MQX)
    lvgl_exclude_file("src/osal/lv_mqx.c")
  endif()
  if(NOT CONFIG_LV_OS_NONE)
    lvgl_exclude_file("src/osal/lv_os_none.c")
  endif()
  if(NOT CONFIG_LV_OS_PTHREAD)
    lvgl_exclude_file("src/osal/lv_pthread.c")
  endif()
  if(NOT CONFIG_LV_OS_RTTHREAD)
    lvgl_exclude_file("src/osal/lv_rtthread.c")
  endif()
  if(NOT CONFIG_LV_OS_WINDOWS)
    lvgl_exclude_file("src/osal/lv_windows.c")
  endif()

  if(NOT CONFIG_LV_USE_BUILTIN_MALLOC AND NOT CONFIG_LV_USE_BUILTIN_STRING
     AND NOT CONFIG_LV_USE_BUILTIN_SPRINTF)
    lvgl_exclude_dir("src/stdlib/builtin")
  endif()
  if(NOT CONFIG_LV_USE_CLIB_MALLOC AND NOT CONFIG_LV_USE_CLIB_STRING
     AND NOT CONFIG_LV_USE_CLIB_SPRINTF)
    lvgl_exclude_dir("src/stdlib/clib")
  endif()
  if(NOT CONFIG_LV_USE_MICROPYTHON_MALLOC)
    lvgl_exclude_dir("src/stdlib/micropython")
  endif()
  if(NOT CONFIG_LV_USE_RTTHREAD_MALLOC)
    lvgl_exclude_dir("src/stdlib/rtthread")
  endif()

  if(NOT CONFIG_LV_USE_FILE_EXPLORER)
    lvgl_exclude_dir("src/others/file_explorer")
  endif()
  if(NOT CONFIG_LV_USE_FRAGMENT)
    lvgl_exclude_dir("src/others/fragment")
  endif()
  if(NOT CONFIG_LV_USE_GRIDNAV)
    lvgl_exclude_dir("src/others/gridnav")
  endif()
  if(NOT CONFIG_LV_USE_IME_PINYIN)
    lvgl_exclude_dir("src/others/ime")
  endif()
  if(NOT CONFIG_LV_USE_IMGFONT)
    lvgl_exclude_dir("src/others/imgfont")
  endif()
  if(NOT CONFIG_LV_USE_MONKEY)
    lvgl_exclude_dir("src/others/monkey")
  endif()
  if(NOT CONFIG_LV_USE_OBSERVER)
    lvgl_exclude_dir("src/others/observer")
  endif()
  if(NOT CONFIG_LV_USE_SNAPSHOT)
    lvgl_exclude_dir("src/others/snapshot")
  endif()
  if(NOT CONFIG_LV_USE_SYSMON)
    lvgl_exclude_dir("src/others/sysmon")
  endif()
  if(NOT CONFIG_LV_USE_VG_LITE_THORVG)
    lvgl_exclude_dir("src/others/vg_lite_tvg")
  endif()

  if(NOT CONFIG_LV_USE_THEME_DEFAULT)
    lvgl_exclude_dir("src/themes/default")
  endif()
  if(NOT CONFIG_LV_USE_THEME_MONO)
    lvgl_exclude_dir("src/themes/mono")
  endif()
  if(NOT CONFIG_LV_USE_THEME_SIMPLE)
    lvgl_exclude_dir("src/themes/simple")
  endif()

  if(NOT CONFIG_LV_CALENDAR_CHINESE)
    lvgl_exclude_file("src/widgets/calendar/lv_calendar_chinese.c")
  endif()
  if(NOT CONFIG_LV_USE_LOTTIE)
    lvgl_exclude_dir("src/widgets/lottie")
  endif()
  if(NOT CONFIG_LV_USE_OBJ_PROPERTY)
    lvgl_exclude_dir("src/widgets/property")
  endif()

  nuttx_add_library(lvgl)

  target_sources(lvgl PRIVATE ${SRCS})
  target_include_directories(lvgl PRIVATE ${LVGL_DIR})
  target_compile_options(lvgl PRIVATE -Wno-shadow)

  if(NOT CONFIG_LV_ASSERT_HANDLER_INCLUDE STREQUAL "")
    target_compile_definitions(lvgl PRIVATE "LV_ASSERT_HANDLER=ASSERT(0)\;")
  endif()

  if(CONFIG_LV_USE_FREETYPE)
    if(NOT CONFIG_LIB_FREETYPE)
      message(WARNING "LIB_FREETYPE is not enabled")
    endif()
    nuttx_add_dependencies(TARGET lvgl DEPENDS freetype)
  endif()

  if(CONFIG_LV_USE_LIBPNG)
    if(NOT CONFIG_LIB_PNG)
      message(WARNING "LIB_PNG is not enabled")
    endif()
    nuttx_add_dependencies(TARGET lvgl DEPENDS png_static)
  endif()

  if(CONFIG_LV_USE_LIBWEBP)
    if(NOT CONFIG_LIB_WEBP)
      message(WARNING "LIB_WEBP is not enabled")
    endif()
    nuttx_add_dependencies(TARGET lvgl DEPENDS webpdecoder)
  endif()

  if(CONFIG_LV_USE_FFMPEG)
    message(FATAL_ERROR "FFMPEG is not supported yet")
  endif()

  if(CONFIG_LV_USE_RLOTTIE)
    message(FATAL_ERROR "RLOTTIE is not supported yet")
  endif()

  if(CONFIG_LV_USE_LIBJPEG_TURBO)
    if(NOT CONFIG_LIB_JPEG_TURBO)
      message(WARNING "LIB_JPEG_TURBO is not enabled")
    endif()
    nuttx_add_dependencies(TARGET lvgl DEPENDS turbojpeg-static)
  endif()

  if(NOT ${CONFIG_LV_OPTLEVEL} STREQUAL "")
    add_compile_options(${CONFIG_LV_OPTLEVEL})
  endif()

  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}
             ${CMAKE_CURRENT_LIST_DIR}/lvgl)

  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_COMPILE_OPTIONS -DLV_USE_DEV_VERSION=1)

endif()
