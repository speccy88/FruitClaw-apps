/****************************************************************************
 * apps/examples/lvgldemo/lvgldemo.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/boardctl.h>

#include <lvgl/lvgl.h>
#include <lvgl/demos/lv_demos.h>
#ifdef CONFIG_INPUT_KEYBOARD
#include <nuttx/input/keyboard.h>
#include <nuttx/input/kbd_codec.h>
#endif
#ifdef CONFIG_INPUT_MOUSE
#include <nuttx/input/mouse.h>
#endif
#ifdef CONFIG_LV_USE_NUTTX_LIBUV
#include <uv.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_INPUT_MOUSE
#ifndef CONFIG_EXAMPLES_LVGLDEMO_MOUSE_DEVPATH
#define CONFIG_EXAMPLES_LVGLDEMO_MOUSE_DEVPATH "/dev/mouse0"
#endif

#ifndef CONFIG_EXAMPLES_LVGLDEMO_MOUSE_WHEEL_STEP
#define CONFIG_EXAMPLES_LVGLDEMO_MOUSE_WHEEL_STEP 12
#endif

#define LVGLDEMO_MOUSE_RETRY_MS 1000
#endif

#ifdef CONFIG_INPUT_KEYBOARD
#ifndef CONFIG_EXAMPLES_LVGLDEMO_KEYBOARD_DEVPATH
#define CONFIG_EXAMPLES_LVGLDEMO_KEYBOARD_DEVPATH "/dev/kbd0"
#endif

#define LVGLDEMO_KEYBOARD_RETRY_MS 1000
#define LVGLDEMO_KEYBOARD_MOD_LSHIFT 0x01
#define LVGLDEMO_KEYBOARD_MOD_RSHIFT 0x02

#ifdef CONFIG_HIDKBD_INPUT_DOSKEYS
#define LVGLDEMO_DOS_KEY_RIGHTARROW 0xae
#define LVGLDEMO_DOS_KEY_LEFTARROW  0xac
#define LVGLDEMO_DOS_KEY_UPARROW    0xad
#define LVGLDEMO_DOS_KEY_DOWNARROW  0xaf
#define LVGLDEMO_DOS_KEY_ESCAPE     27
#define LVGLDEMO_DOS_KEY_ENTER      13
#define LVGLDEMO_DOS_KEY_TAB        9
#define LVGLDEMO_DOS_KEY_BACKSPACE  0x7f
#define LVGLDEMO_DOS_KEY_RSHIFT     (0x80 + 0x36)
#define LVGLDEMO_DOS_KEY_CAPSLOCK   (0x80 + 0x3a)
#define LVGLDEMO_DOS_KEY_HOME       (0x80 + 0x47)
#define LVGLDEMO_DOS_KEY_END        (0x80 + 0x4f)
#define LVGLDEMO_DOS_KEY_PGUP       (0x80 + 0x49)
#define LVGLDEMO_DOS_KEY_PGDN       (0x80 + 0x51)
#define LVGLDEMO_DOS_KEY_INS        (0x80 + 0x52)
#define LVGLDEMO_DOS_KEY_DEL        (0x80 + 0x53)
#endif
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

#ifdef CONFIG_INPUT_MOUSE
struct lvgldemo_mouse_s
{
  int fd;
  uint8_t buttons;
  int16_t wheel;
  uint32_t last_open;
  bool have_wheel;
  lv_point_t last;
  lv_indev_t *indev;
};
#endif

#ifdef CONFIG_INPUT_KEYBOARD
struct lvgldemo_keyboard_s
{
  int fd;
  uint8_t modifiers;
  uint32_t key;
  uint32_t last_open;
  bool capslock;
  lv_indev_state_t state;
  lv_indev_t *indev;
  lv_group_t *group;
};
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_INPUT_MOUSE
static struct lvgldemo_mouse_s g_lvgldemo_mouse =
{
  .fd = -1
};
#endif

#ifdef CONFIG_INPUT_KEYBOARD
static struct lvgldemo_keyboard_s g_lvgldemo_keyboard =
{
  .fd = -1,
  .state = LV_INDEV_STATE_RELEASED
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_INPUT_MOUSE
static int32_t lvgldemo_clip_axis(int32_t value, int32_t limit)
{
  if (value < 0)
    {
      return 0;
    }

  if (value >= limit)
    {
      return limit - 1;
    }

  return value;
}

static void lvgldemo_mouse_open(FAR struct lvgldemo_mouse_s *mouse)
{
  mouse->last_open = lv_tick_get();

  mouse->fd = open(CONFIG_EXAMPLES_LVGLDEMO_MOUSE_DEVPATH,
                   O_RDONLY | O_NONBLOCK);
  if (mouse->fd < 0)
    {
      return;
    }

  mouse->have_wheel = false;
  LV_LOG_INFO("LVGL mouse input attached to %s",
              CONFIG_EXAMPLES_LVGLDEMO_MOUSE_DEVPATH);
}

static FAR lv_obj_t *lvgldemo_mouse_find_scrollable(FAR lv_obj_t *obj)
{
  while (obj != NULL)
    {
      if ((lv_obj_get_scroll_dir(obj) & LV_DIR_VER) != 0 &&
          (lv_obj_get_scroll_top(obj) > 0 ||
           lv_obj_get_scroll_bottom(obj) > 0))
        {
          return obj;
        }

      obj = lv_obj_get_parent(obj);
    }

  return NULL;
}

static void lvgldemo_mouse_scroll(FAR struct lvgldemo_mouse_s *mouse,
                                  int16_t diff)
{
  FAR lv_obj_t *obj;

  if (diff == 0)
    {
      return;
    }

  obj = lv_indev_search_obj(lv_screen_active(), &mouse->last);
  obj = lvgldemo_mouse_find_scrollable(obj);
  if (obj == NULL)
    {
      return;
    }

  lv_obj_scroll_by_bounded(obj, 0,
                           -(int32_t)diff *
                           CONFIG_EXAMPLES_LVGLDEMO_MOUSE_WHEEL_STEP,
                           LV_ANIM_OFF);
}

static void lvgldemo_mouse_read(lv_indev_t *indev, lv_indev_data_t *data)
{
  FAR struct lvgldemo_mouse_s *mouse;
  lv_display_t *disp;
  struct mouse_report_s report;
  ssize_t nread;
  int errcode;

  mouse = lv_indev_get_driver_data(indev);

  if (mouse->fd < 0 &&
      lv_tick_get() - mouse->last_open >= LVGLDEMO_MOUSE_RETRY_MS)
    {
      lvgldemo_mouse_open(mouse);
    }

  while (mouse->fd >= 0)
    {
      nread = read(mouse->fd, &report, sizeof(report));
      if (nread != sizeof(report))
        {
          if (nread < 0)
            {
              errcode = errno;
              if (errcode != EAGAIN && errcode != EINTR)
                {
                  close(mouse->fd);
                  mouse->fd = -1;
                  mouse->last_open = lv_tick_get();
                }
            }

          break;
        }

      disp = lv_indev_get_display(indev);
      mouse->last.x =
        lvgldemo_clip_axis(report.x,
                           lv_display_get_horizontal_resolution(disp));
      mouse->last.y =
        lvgldemo_clip_axis(report.y,
                           lv_display_get_vertical_resolution(disp));
      mouse->buttons = report.buttons;

#ifdef CONFIG_INPUT_MOUSE_WHEEL
      if (mouse->have_wheel)
        {
          lvgldemo_mouse_scroll(mouse, report.wheel - mouse->wheel);
        }

      mouse->wheel = report.wheel;
      mouse->have_wheel = true;
#endif
    }

  data->point = mouse->last;
  data->state = (mouse->buttons & MOUSE_BUTTON_1) != 0 ?
                LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static void lvgldemo_mouse_init(lv_display_t *disp)
{
  FAR struct lvgldemo_mouse_s *mouse = &g_lvgldemo_mouse;
  FAR lv_obj_t *cursor;

  mouse->last.x = lv_display_get_horizontal_resolution(disp) / 2;
  mouse->last.y = lv_display_get_vertical_resolution(disp) / 2;

  mouse->indev = lv_indev_create();
  if (mouse->indev == NULL)
    {
      LV_LOG_WARN("Failed to create LVGL mouse input");
      return;
    }

  lv_indev_set_type(mouse->indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(mouse->indev, lvgldemo_mouse_read);
  lv_indev_set_driver_data(mouse->indev, mouse);
  lv_indev_set_display(mouse->indev, disp);

  cursor = lv_obj_create(lv_display_get_layer_sys(disp));
  lv_obj_remove_style_all(cursor);
  lv_obj_remove_flag(cursor, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_size(cursor, 7, 7);
  lv_obj_set_style_radius(cursor, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(cursor, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(cursor, lv_color_black(), 0);
  lv_obj_set_style_border_width(cursor, 1, 0);
  lv_obj_set_style_border_color(cursor, lv_color_white(), 0);
  lv_indev_set_cursor(mouse->indev, cursor);

  lvgldemo_mouse_open(mouse);
}

static void lvgldemo_mouse_deinit(void)
{
  FAR struct lvgldemo_mouse_s *mouse = &g_lvgldemo_mouse;

  if (mouse->fd >= 0)
    {
      close(mouse->fd);
      mouse->fd = -1;
    }

  if (mouse->indev != NULL)
    {
      lv_indev_delete(mouse->indev);
      mouse->indev = NULL;
    }
}
#endif

#ifdef CONFIG_INPUT_KEYBOARD
static void lvgldemo_keyboard_hide_software(lv_obj_t *obj);

static bool lvgldemo_keyboard_shifted(
    FAR const struct lvgldemo_keyboard_s *keyboard)
{
  return keyboard->modifiers != 0;
}

static uint32_t lvgldemo_keyboard_shift_ascii(
    FAR const struct lvgldemo_keyboard_s *keyboard, uint32_t code)
{
  bool shifted = lvgldemo_keyboard_shifted(keyboard);

  if (code >= 'a' && code <= 'z')
    {
      if (shifted != keyboard->capslock)
        {
          return code - 'a' + 'A';
        }

      return code;
    }

  if (!shifted)
    {
      return code;
    }

  switch (code)
    {
      case '1':
        return '!';

      case '2':
        return '@';

      case '3':
        return '#';

      case '4':
        return '$';

      case '5':
        return '%';

      case '6':
        return '^';

      case '7':
        return '&';

      case '8':
        return '*';

      case '9':
        return '(';

      case '0':
        return ')';

      case '-':
        return '_';

      case '=':
        return '+';

      case '[':
        return '{';

      case ']':
        return '}';

      case '\\':
        return '|';

      case ';':
        return ':';

      case '\'':
        return '"';

      case '`':
        return '~';

      case ',':
        return '<';

      case '.':
        return '>';

      case '/':
        return '?';

      default:
        return code;
    }
}

static void lvgldemo_keyboard_modifier(
    FAR struct lvgldemo_keyboard_s *keyboard, uint32_t code, uint32_t type)
{
  uint8_t modifier = 0;

  switch (code)
    {
      case XK_Shift_L:
        modifier = LVGLDEMO_KEYBOARD_MOD_LSHIFT;
        break;

      case XK_Shift_R:
        modifier = LVGLDEMO_KEYBOARD_MOD_RSHIFT;
        break;

#ifdef CONFIG_HIDKBD_INPUT_DOSKEYS
      case LVGLDEMO_DOS_KEY_RSHIFT:
        modifier = LVGLDEMO_KEYBOARD_MOD_RSHIFT;
        break;
#endif

      default:
        return;
    }

  if (type == KEYBOARD_PRESS)
    {
      keyboard->modifiers |= modifier;
    }
  else
    {
      keyboard->modifiers &= ~modifier;
    }
}

static bool lvgldemo_keyboard_keycode(
    FAR struct lvgldemo_keyboard_s *keyboard,
    FAR const struct keyboard_event_s *event, FAR uint32_t *key)
{
  lvgldemo_keyboard_modifier(keyboard, event->code, event->type);

  if ((event->code == KEYCODE_CAPSLOCK
#ifdef CONFIG_HIDKBD_INPUT_DOSKEYS
       || event->code == LVGLDEMO_DOS_KEY_CAPSLOCK
#endif
      ) && event->type == KEYBOARD_PRESS)
    {
      keyboard->capslock = !keyboard->capslock;
      return false;
    }

#ifdef CONFIG_HIDKBD_INPUT_DOSKEYS
  switch (event->code)
    {
      case LVGLDEMO_DOS_KEY_RSHIFT:
        return false;

      case LVGLDEMO_DOS_KEY_ENTER:
        *key = LV_KEY_ENTER;
        return true;

      case LVGLDEMO_DOS_KEY_ESCAPE:
        *key = LV_KEY_ESC;
        return true;

      case LVGLDEMO_DOS_KEY_TAB:
        *key = lvgldemo_keyboard_shifted(keyboard) ?
               LV_KEY_PREV : LV_KEY_NEXT;
        return true;

      case LVGLDEMO_DOS_KEY_BACKSPACE:
        *key = LV_KEY_BACKSPACE;
        return true;

      case LVGLDEMO_DOS_KEY_DEL:
        *key = LV_KEY_DEL;
        return true;

      case LVGLDEMO_DOS_KEY_HOME:
        *key = LV_KEY_HOME;
        return true;

      case LVGLDEMO_DOS_KEY_END:
        *key = LV_KEY_END;
        return true;

      case LVGLDEMO_DOS_KEY_LEFTARROW:
        *key = LV_KEY_LEFT;
        return true;

      case LVGLDEMO_DOS_KEY_RIGHTARROW:
        *key = LV_KEY_RIGHT;
        return true;

      case LVGLDEMO_DOS_KEY_UPARROW:
      case LVGLDEMO_DOS_KEY_PGUP:
        *key = LV_KEY_UP;
        return true;

      case LVGLDEMO_DOS_KEY_DOWNARROW:
      case LVGLDEMO_DOS_KEY_PGDN:
        *key = LV_KEY_DOWN;
        return true;

      default:
        break;
    }
#endif

  switch (event->code)
    {
      case XK_Shift_L:
      case XK_Shift_R:
      case XK_Control_L:
      case XK_Control_R:
      case XK_Alt_L:
      case XK_Alt_R:
      case XK_Meta_L:
      case XK_Meta_R:
        return false;

      case KEYCODE_ENTER:
      case XK_Return:
      case XK_KP_Enter:
        *key = LV_KEY_ENTER;
        return true;

      case XK_Escape:
        *key = LV_KEY_ESC;
        return true;

      case XK_Tab:
      case XK_KP_Tab:
        *key = lvgldemo_keyboard_shifted(keyboard) ?
               LV_KEY_PREV : LV_KEY_NEXT;
        return true;

      case XK_BackSpace:
      case KEYCODE_BACKDEL:
        *key = LV_KEY_BACKSPACE;
        return true;

      case XK_Delete:
      case KEYCODE_FWDDEL:
        *key = LV_KEY_DEL;
        return true;

      case KEYCODE_HOME:
      case XK_Home:
      case XK_KP_Home:
        *key = LV_KEY_HOME;
        return true;

      case KEYCODE_END:
      case XK_End:
      case XK_KP_End:
        *key = LV_KEY_END;
        return true;

      case KEYCODE_LEFT:
      case XK_Left:
      case XK_KP_Left:
        *key = LV_KEY_LEFT;
        return true;

      case KEYCODE_RIGHT:
      case XK_Right:
      case XK_KP_Right:
        *key = LV_KEY_RIGHT;
        return true;

      case KEYCODE_UP:
      case XK_Up:
      case XK_KP_Up:
      case KEYCODE_PAGEUP:
      case XK_Page_Up:
      case XK_KP_Page_Up:
        *key = LV_KEY_UP;
        return true;

      case KEYCODE_DOWN:
      case XK_Down:
      case XK_KP_Down:
      case KEYCODE_PAGEDOWN:
      case XK_Page_Down:
      case XK_KP_Page_Down:
        *key = LV_KEY_DOWN;
        return true;

      default:
        if (event->code >= 0x20 && event->code <= 0x7e)
          {
            *key = lvgldemo_keyboard_shift_ascii(keyboard, event->code);
            return true;
          }

        return false;
    }
}

static FAR lv_group_t *lvgldemo_keyboard_group(
    FAR struct lvgldemo_keyboard_s *keyboard)
{
  if (keyboard->group == NULL)
    {
      keyboard->group = lv_group_create();
      if (keyboard->group == NULL)
        {
          return NULL;
        }
    }

  lv_group_set_default(keyboard->group);
  return keyboard->group;
}

static void lvgldemo_keyboard_open(FAR struct lvgldemo_keyboard_s *keyboard)
{
  keyboard->last_open = lv_tick_get();

  keyboard->fd = open(CONFIG_EXAMPLES_LVGLDEMO_KEYBOARD_DEVPATH,
                      O_RDONLY | O_NONBLOCK);
  if (keyboard->fd < 0)
    {
      return;
    }

  LV_LOG_INFO("LVGL keyboard input attached to %s",
              CONFIG_EXAMPLES_LVGLDEMO_KEYBOARD_DEVPATH);
}

static void lvgldemo_keyboard_read(lv_indev_t *indev,
                                   lv_indev_data_t *data)
{
  FAR struct lvgldemo_keyboard_s *keyboard;
  struct keyboard_event_s event;
  uint32_t key;
  ssize_t nread;
  int errcode;

  keyboard = lv_indev_get_driver_data(indev);

  if (keyboard->fd < 0 &&
      lv_tick_get() - keyboard->last_open >= LVGLDEMO_KEYBOARD_RETRY_MS)
    {
      lvgldemo_keyboard_open(keyboard);
    }

  while (keyboard->fd >= 0)
    {
      nread = read(keyboard->fd, &event, sizeof(event));
      if (nread != sizeof(event))
        {
          if (nread < 0)
            {
              errcode = errno;
              if (errcode != EAGAIN && errcode != EINTR)
                {
                  close(keyboard->fd);
                  keyboard->fd = -1;
                  keyboard->last_open = lv_tick_get();
                }
            }

          break;
        }

      if (lvgldemo_keyboard_keycode(keyboard, &event, &key))
        {
          lvgldemo_keyboard_hide_software(lv_screen_active());
          keyboard->key = key;
          keyboard->state = event.type == KEYBOARD_PRESS ?
                            LV_INDEV_STATE_PRESSED :
                            LV_INDEV_STATE_RELEASED;
          data->continue_reading = true;
          break;
        }
    }

  data->key = keyboard->key;
  data->state = keyboard->state;
}

static void lvgldemo_keyboard_init(lv_display_t *disp)
{
  FAR struct lvgldemo_keyboard_s *keyboard = &g_lvgldemo_keyboard;
  FAR lv_group_t *group;

  group = lvgldemo_keyboard_group(keyboard);
  if (group == NULL)
    {
      LV_LOG_WARN("Failed to create LVGL keyboard group");
      return;
    }

  keyboard->indev = lv_indev_create();
  if (keyboard->indev == NULL)
    {
      LV_LOG_WARN("Failed to create LVGL keyboard input");
      return;
    }

  lv_indev_set_type(keyboard->indev, LV_INDEV_TYPE_KEYPAD);
  lv_indev_set_read_cb(keyboard->indev, lvgldemo_keyboard_read);
  lv_indev_set_driver_data(keyboard->indev, keyboard);
  lv_indev_set_display(keyboard->indev, disp);
  lv_indev_set_group(keyboard->indev, group);

  lvgldemo_keyboard_open(keyboard);
}

static void lvgldemo_keyboard_textarea_event(lv_event_t *event)
{
  lv_event_code_t code = lv_event_get_code(event);

  if (code == LV_EVENT_FOCUSED ||
      code == LV_EVENT_READY ||
      code == LV_EVENT_CANCEL ||
      code == LV_EVENT_DEFOCUSED)
    {
      lvgldemo_keyboard_hide_software(lv_obj_get_screen(
          lv_event_get_target(event)));
    }
}

static void lvgldemo_keyboard_hide_software(lv_obj_t *obj)
{
  uint32_t count;
  uint32_t i;

  if (obj == NULL)
    {
      return;
    }

  if (lv_obj_get_screen(obj) == obj)
    {
      count = lv_obj_get_child_count(obj);
      for (i = 0; i < count; i++)
        {
          lv_obj_t *child = lv_obj_get_child(obj, i);

#if LV_USE_KEYBOARD
          if (lv_obj_check_type(child, &lv_keyboard_class))
            {
              continue;
            }
#endif

          lv_obj_set_height(child, LV_VER_RES);
        }
    }

#if LV_USE_KEYBOARD
  if (lv_obj_check_type(obj, &lv_keyboard_class))
    {
      lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
#endif

  count = lv_obj_get_child_count(obj);
  for (i = 0; i < count; i++)
    {
      lvgldemo_keyboard_hide_software(lv_obj_get_child(obj, i));
    }
}

static void lvgldemo_keyboard_prefer_hardware(lv_obj_t *obj)
{
  uint32_t count;
  uint32_t i;

  if (obj == NULL)
    {
      return;
    }

#if LV_USE_KEYBOARD
  if (lv_obj_check_type(obj, &lv_keyboard_class))
    {
      lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
#endif

  if (lv_obj_check_type(obj, &lv_textarea_class))
    {
      lv_obj_add_event_cb(obj, lvgldemo_keyboard_textarea_event,
                          LV_EVENT_ALL, NULL);
    }

  count = lv_obj_get_child_count(obj);
  for (i = 0; i < count; i++)
    {
      lvgldemo_keyboard_prefer_hardware(lv_obj_get_child(obj, i));
    }
}

static void lvgldemo_keyboard_deinit(void)
{
  FAR struct lvgldemo_keyboard_s *keyboard = &g_lvgldemo_keyboard;

  if (keyboard->fd >= 0)
    {
      close(keyboard->fd);
      keyboard->fd = -1;
    }

  if (keyboard->indev != NULL)
    {
      lv_indev_delete(keyboard->indev);
      keyboard->indev = NULL;
    }

  if (keyboard->group != NULL)
    {
      lv_group_delete(keyboard->group);
      keyboard->group = NULL;
    }
}
#endif

#ifdef CONFIG_LV_USE_NUTTX_LIBUV
static void lv_nuttx_uv_loop(uv_loop_t *loop, lv_nuttx_result_t *result)
{
  lv_nuttx_uv_t uv_info;
  void *data;

  uv_loop_init(loop);

  lv_memset(&uv_info, 0, sizeof(uv_info));
  uv_info.loop = loop;
  uv_info.disp = result->disp;
  uv_info.indev = result->indev;
#ifdef CONFIG_UINPUT_TOUCH
  uv_info.uindev = result->utouch_indev;
#endif

  data = lv_nuttx_uv_init(&uv_info);
  uv_run(loop, UV_RUN_DEFAULT);
  lv_nuttx_uv_deinit(&data);
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main or lv_demos_main
 *
 * Description:
 *
 * Input Parameters:
 *   Standard argc and argv
 *
 * Returned Value:
 *   Zero on success; a positive, non-zero value on failure.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  lv_nuttx_dsc_t info;
  lv_nuttx_result_t result;

#ifdef CONFIG_LV_USE_NUTTX_LIBUV
  uv_loop_t ui_loop;
  lv_memzero(&ui_loop, sizeof(ui_loop));
#endif

  if (lv_is_initialized())
    {
      LV_LOG_ERROR("LVGL already initialized! aborting.");
      return -1;
    }

  lv_init();

  lv_nuttx_dsc_init(&info);

#ifdef CONFIG_LV_USE_NUTTX_LCD
  info.fb_path = "/dev/lcd0";
#endif

#ifdef CONFIG_INPUT_TOUCHSCREEN
  info.input_path = CONFIG_EXAMPLES_LVGLDEMO_INPUT_DEVPATH;
#endif

  lv_nuttx_init(&info, &result);

  if (result.disp == NULL)
    {
      LV_LOG_ERROR("lv_demos initialization failure!");
      return 1;
    }

#ifdef CONFIG_INPUT_MOUSE
  lvgldemo_mouse_init(result.disp);
#endif

#ifdef CONFIG_INPUT_KEYBOARD
  lvgldemo_keyboard_init(result.disp);
#endif

  if (!lv_demos_create(&argv[1], argc - 1))
    {
      lv_demos_show_help();

      /* we can add custom demos here */

      goto demo_end;
    }

#ifdef CONFIG_INPUT_KEYBOARD
  lvgldemo_keyboard_prefer_hardware(lv_screen_active());
#endif

#ifdef CONFIG_LV_USE_NUTTX_LIBUV
  lv_nuttx_uv_loop(&ui_loop, &result);
#else
  while (1)
    {
      uint32_t idle;
      idle = lv_timer_handler();

      /* Minimum sleep of 1ms */

      idle = idle ? idle : 1;
      usleep(idle * 1000);
    }
#endif

demo_end:
#ifdef CONFIG_INPUT_KEYBOARD
  lvgldemo_keyboard_deinit();
#endif

#ifdef CONFIG_INPUT_MOUSE
  lvgldemo_mouse_deinit();
#endif
  lv_nuttx_deinit(&result);
  lv_deinit();

  return 0;
}
