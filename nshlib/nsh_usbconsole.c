/****************************************************************************
 * apps/nshlib/nsh_usbconsole.c
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

#include <sys/boardctl.h>
#include <sys/ioctl.h>

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <nuttx/debug.h>

#ifdef CONFIG_SERIAL_TERMIOS
#  include <termios.h>
#endif

#ifdef CONFIG_CDCACM
#  include <nuttx/usb/cdc.h>
#  include <nuttx/usb/cdcacm.h>
#endif

#ifdef CONFIG_PL2303
#  include <nuttx/usb/pl2303.h>
#endif

#include "nsh.h"
#include "nsh_console.h"

#include "netutils/netinit.h"

#ifdef HAVE_USB_CONSOLE

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_usbstdiofd = -1;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_configstdio
 *
 * Description:
 *   Configure standard I/O
 *
 ****************************************************************************/

static void nsh_configstdio(int fd)
{
  /* Dup the fd to create standard fd 0-2 */

  dup2(fd, 0);
  dup2(fd, 1);
  dup2(fd, 2);
}

/****************************************************************************
 * Name: nsh_closeusbstdio
 ****************************************************************************/

static void nsh_closeusbstdio(FAR struct console_stdio_s *pstate)
{
  if (g_usbstdiofd >= 0)
    {
      close(g_usbstdiofd);
      g_usbstdiofd = -1;
    }

  INFD(pstate)  = STDIN_FILENO;
  OUTFD(pstate) = STDOUT_FILENO;
  ERRFD(pstate) = STDERR_FILENO;
}

/****************************************************************************
 * Name: nsh_configusbtermios
 ****************************************************************************/

#ifdef CONFIG_SERIAL_TERMIOS
static void nsh_configusbtermios(int fd)
{
  struct termios cfg;

  if (tcgetattr(fd, &cfg) == 0)
    {
      cfg.c_iflag = 0;
      cfg.c_oflag = OPOST | ONLCR;
      cfg.c_lflag = ISIG;
      cfg.c_cc[VMIN] = 1;
      cfg.c_cc[VTIME] = 0;

      tcsetattr(fd, TCSANOW, &cfg);
    }
}
#endif

/****************************************************************************
 * Name: nsh_configusbstdio
 *
 * Description:
 *   Configure standard I/O and the NSH front-end to use the live USB
 *   descriptor.
 *
 ****************************************************************************/

static void nsh_configusbstdio(FAR struct console_stdio_s *pstate, int fd)
{
  if (g_usbstdiofd >= 0 && g_usbstdiofd != fd)
    {
      close(g_usbstdiofd);
    }

  g_usbstdiofd = fd;

  nsh_configstdio(fd);

#ifdef CONFIG_SERIAL_TERMIOS
  nsh_configusbtermios(fd);
#endif

  INFD(pstate)  = fd;
  OUTFD(pstate) = fd;
  ERRFD(pstate) = fd;
}

/****************************************************************************
 * Name: nsh_waitusbdtr
 *
 * Description:
 *   Wait until the CDC ACM host asserts DTR.
 *
 ****************************************************************************/

#ifdef CONFIG_NSH_USBCONSOLE_WAIT_DTR
static void nsh_waitusbdtr(int fd)
{
  int elapsed = 0;
  int ctrlline;

  for (; ; )
    {
      ctrlline = 0;
      if (ioctl(fd, CAIOC_GETCTRLLINE, (unsigned long)&ctrlline) < 0)
        {
          return;
        }

      if ((ctrlline & CDCACM_UART_DTR) != 0)
        {
          return;
        }

#if CONFIG_NSH_USBCONSOLE_WAIT_DTR_TIMEOUT > 0
      if (elapsed >= CONFIG_NSH_USBCONSOLE_WAIT_DTR_TIMEOUT)
        {
          return;
        }
#endif

      usleep(10000);
      elapsed += 10;
    }
}
#endif

/****************************************************************************
 * Name: nsh_nullstdio
 *
 * Description:
 *   Use /dev/null for standard I/O
 *
 ****************************************************************************/

static int nsh_nullstdio(void)
{
  int fd;

  /* Open /dev/null for read/write access */

  fd = open("/dev/null", O_RDWR);
  if (fd >= 0)
    {
      /* Configure standard I/O to use /dev/null */

      nsh_configstdio(fd);

      /* We can close the original file descriptor now (unless it was one of
       * 0-2)
       */

      if (fd > 2)
        {
          close(fd);
        }

      return OK;
    }

  return fd;
}

/****************************************************************************
 * Name: nsh_waitusbready
 *
 * Description:
 *   Wait for the USB console device to be ready
 *
 ****************************************************************************/

static int nsh_waitusbready(FAR struct console_stdio_s *pstate)
{
#ifdef CONFIG_NSH_USBCONSOLE_WAIT_INPUT
  char inch;
  ssize_t nbytes;
  int nlc;
#endif
  int fd;

  /* Don't start the NSH console until the console device is ready.  Chances
   * are, we get here with no functional console.  The USB console will not
   * be available until the device is connected to the host and until the
   * host-side application opens the connection.
   */

#ifdef CONFIG_NSH_USBCONSOLE_WAIT_INPUT
restart:
#endif

  /* Open the USB serial device for read/write access */

  do
    {
      /* Try to open the console */

      fd = open(CONFIG_NSH_USBCONDEV, O_RDWR);
      if (fd < 0)
        {
          /* ENOTCONN means that the USB device is not yet connected.
           * Anything else is bad.
           */

          DEBUGASSERT(errno == ENOTCONN);

          /* Sleep a bit and try again */

          sleep(2);
        }
    }
  while (fd < 0);

#ifndef CONFIG_NSH_USBCONSOLE_WAIT_INPUT
  /* The USB serial device is configured.  Start the session immediately and
   * let the command-line reader block until the host sends input.
   */

#ifdef CONFIG_NSH_USBCONSOLE_WAIT_DTR
  nsh_waitusbdtr(fd);
#endif

#if defined(CONFIG_NSH_USBCONSOLE_CONNECT_DELAY) && \
    CONFIG_NSH_USBCONSOLE_CONNECT_DELAY > 0
  usleep(CONFIG_NSH_USBCONSOLE_CONNECT_DELAY * 1000);
#endif

  nsh_configusbstdio(pstate, fd);

  return OK;
#else
  /* Now wait until we successfully read a carriage return a few times.
   * That is a sure way of know that there is something at the other end of
   * the USB serial connection that is ready to talk with us.  The user needs
   * to hit ENTER a few times to get things started.
   */

  nlc = 0;
  do
    {
      /* Read one byte */

      inch = 0;
      nbytes = read(fd, &inch, 1);

      /* Is it a carriage return (or maybe a newline)? */

      if (nbytes == 1 && (inch == '\n' || inch == '\r'))
        {
          /* Yes.. increment the count */

          nlc++;
        }
      else
        {
          /* No.. Reset the count.  We need to see 3 in a row to continue. */

          nlc = 0;

          /* If a read error occurred (nbytes < 0) or an end-of-file was
           * encountered (nbytes == 0), then close the driver and start
           * over.
           */

          if (nbytes <= 0)
            {
              close(fd);
              goto restart;
            }
        }
    }
  while (nlc < 3);

  /* Configure standard I/O */

  nsh_configusbstdio(pstate, fd);

  return OK;
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_consolemain (USB console version)
 *
 * Description:
 *   This interface may be called or started with task_start to start a
 *   single an NSH instance that operates on stdin and stdout.  This
 *   function does not return.
 *
 *   This function handles generic /dev/console character devices, or
 *   special USB console devices.  The USB console requires some special
 *   operations to handle the cases where the session is lost when the
 *   USB device is unplugged and restarted when the USB device is plugged
 *   in again.
 *
 * Input Parameters:
 *   Standard task start-up arguments.  These are not used.  argc may be
 *   zero and argv may be NULL.
 *
 * Returned Values:
 *   This function does not return nor does it ever exit (unless the user
 *   executes the NSH exit command).
 *
 ****************************************************************************/

int nsh_consolemain(int argc, FAR char *argv[])
{
  FAR struct console_stdio_s *pstate = nsh_newconsole(true);
  struct boardioc_usbdev_ctrl_s ctrl;
  FAR void *handle;
  int ret;

  DEBUGASSERT(pstate);

  /* Initialize the USB serial driver */

#if defined(CONFIG_PL2303) || defined(CONFIG_CDCACM)
#if defined(CONFIG_USBDEV_COMPOSITE)

  ctrl.usbdev   = BOARDIOC_USBDEV_COMPOSITE;
  ctrl.action   = BOARDIOC_USBDEV_INITIALIZE;
  ctrl.instance = 0;
  ctrl.config   = 0;
  ctrl.handle   = NULL;
  ret = boardctl(BOARDIOC_USBDEV_CONTROL, (uintptr_t)&ctrl);

  ctrl.usbdev   = BOARDIOC_USBDEV_COMPOSITE;
  ctrl.action   = BOARDIOC_USBDEV_CONNECT;
  ctrl.instance = 0;
  ctrl.config   = 0;
  ctrl.handle   = &handle;

#elif defined(CONFIG_CDCACM)

  ctrl.usbdev   = BOARDIOC_USBDEV_CDCACM;
  ctrl.action   = BOARDIOC_USBDEV_CONNECT;
  ctrl.instance = CONFIG_NSH_USBDEV_MINOR;
  ctrl.handle   = &handle;

#else

  ctrl.usbdev   = BOARDIOC_USBDEV_PL2303;
  ctrl.action   = BOARDIOC_USBDEV_CONNECT;
  ctrl.instance = CONFIG_NSH_USBDEV_MINOR;
  ctrl.handle   = &handle;

#endif

  ret = boardctl(BOARDIOC_USBDEV_CONTROL, (uintptr_t)&ctrl);
  UNUSED(ret); /* Eliminate warning if not used */
  DEBUGASSERT(ret == OK);
#endif

  /* Configure to use /dev/null if we do not have a valid console. */

#ifndef CONFIG_DEV_CONSOLE
  nsh_nullstdio();
#endif

  /* Now loop, executing creating a session for each USB connection */

  for (; ; )
    {
      /* Wait for the USB to be connected to the host and switch
       * standard I/O to the USB serial device.
       */

      ret = nsh_waitusbready(pstate);
      UNUSED(ret); /* Eliminate warning if not used */
      DEBUGASSERT(ret == OK);

      /* Execute the session */

      nsh_session(pstate, NSH_LOGIN_LOCAL, argc, argv);

      /* Switch to /dev/null because we probably no longer have a
       * valid console device.
       */

      nsh_nullstdio();
      nsh_closeusbstdio(pstate);
    }
}

#endif /* HAVE_USB_CONSOLE */
