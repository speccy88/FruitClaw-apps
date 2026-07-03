/* SPDX-License-Identifier: Apache-2.0 */

#include "fruitclaw.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#ifdef CONFIG_NETUTILS_HTTPD_CGIPATH
#  include "netutils/httpd.h"
#endif

#ifndef CONFIG_FRUITCLAW_WEB_HOME_MAX_BYTES
#  define CONFIG_FRUITCLAW_WEB_HOME_MAX_BYTES 4096
#endif

#define FC_WEB_HOME_LEAF "www/home.md"

static const char g_default_home[] =
  "# FruitClaw\n\n"
  "FruitClaw is running on this Adafruit Fruit Jam RP2350.\n\n"
  "- Open `/docs/` for the full manual.\n"
  "- Use MCP at `/mcp` for tool calls.\n"
  "- Update this page with the `web.home.write` tool or by editing "
  "`www/home.md` under the active FruitClaw data root.\n";

static bool g_web_registered;

static int fc_web_home_path(char *out, size_t out_len)
{
  return fc_data_path(FC_WEB_HOME_LEAF, out, out_len);
}

int fc_web_home_read(char *out, size_t out_len, bool *custom)
{
  char path[FC_PATH_LEN];
  int ret;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (custom != NULL)
    {
      *custom = false;
    }

  ret = fc_web_home_path(path, sizeof(path));
  if (ret == 0)
    {
      ret = fc_read_text_file(path, out, out_len, false);
      if (ret == 0)
        {
          if (custom != NULL)
            {
              *custom = true;
            }

          return 0;
        }
    }

  fc_strlcpy(out, g_default_home, out_len);
  return 0;
}

int fc_web_home_write(const char *markdown)
{
  char dir[FC_PATH_LEN];
  char path[FC_PATH_LEN];
  size_t len;
  int ret;

  if (markdown == NULL)
    {
      return -EINVAL;
    }

  len = strlen(markdown);
  if (len > CONFIG_FRUITCLAW_WEB_HOME_MAX_BYTES)
    {
      return -EFBIG;
    }

  ret = fc_data_path("www", dir, sizeof(dir));
  if (ret < 0)
    {
      return ret;
    }

  ret = fc_mkdir_p(dir);
  if (ret < 0)
    {
      return ret;
    }

  ret = fc_web_home_path(path, sizeof(path));
  if (ret < 0)
    {
      return ret;
    }

  return fc_write_text_file_atomic(path, markdown);
}

#ifdef CONFIG_NETUTILS_HTTPD_CGIPATH
static void fc_web_home_http_handler(struct httpd_state *pstate, char *path)
{
  static const char extra_headers[] =
    "Allow: GET, OPTIONS\r\n"
    "Cache-Control: no-cache\r\n";
  char body[CONFIG_FRUITCLAW_WEB_HOME_MAX_BYTES + 1];
  int ret;

  (void)path;
  fc_guard_session_heartbeat("web-home");

  if (pstate->ht_method == HTTPD_METHOD_OPTIONS)
    {
      httpd_send_response(pstate, 204, "text/plain", extra_headers,
                          NULL, 0);
      return;
    }

  if (pstate->ht_method != HTTPD_METHOD_GET)
    {
      httpd_send_response(pstate, 405, "text/plain", extra_headers,
                          NULL, 0);
      return;
    }

  ret = fc_web_home_read(body, sizeof(body), NULL);
  if (ret < 0)
    {
      httpd_send_response(pstate, 500, "text/plain", extra_headers,
                          "home read failed\n", 17);
      return;
    }

  httpd_send_response(pstate, 200, "text/markdown; charset=utf-8",
                      extra_headers, body, strlen(body));
}

HTTPD_CGI_CALL(g_fc_web_home_cgi, "/site/home.md",
               fc_web_home_http_handler);
#endif

int fc_web_register_http(void)
{
#ifdef CONFIG_NETUTILS_HTTPD_CGIPATH
  if (g_web_registered)
    {
      return 0;
    }

  httpd_cgi_register(&g_fc_web_home_cgi);
  g_web_registered = true;
  FC_LOGI("web home endpoint registered at /site/home.md");
#endif

  return 0;
}
