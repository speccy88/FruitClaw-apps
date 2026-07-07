/****************************************************************************
 * apps/system/telnetd/telnetd.c
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

#include <netinet/in.h>
#include <sys/socket.h>

#include "netutils/telnetd.h"
#include "nshlib/nshlib.h"

#ifndef CONFIG_SYSTEM_TELNETD_PID_PATH
#  define CONFIG_SYSTEM_TELNETD_PID_PATH "/tmp/telnetd.pid"
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int telnetd_write_pid(void)
{
  char buf[24];
  int fd;
  int len;
  ssize_t n;

  fd = open(CONFIG_SYSTEM_TELNETD_PID_PATH, O_WRONLY | O_CREAT | O_TRUNC,
            0664);
  if (fd < 0)
    {
      return -errno;
    }

  len = snprintf(buf, sizeof(buf), "%ld\n", (long)getpid());
  n = write(fd, buf, len);
  close(fd);
  return n == len ? 0 : -EIO;
}

static int telnetd_read_pid(pid_t *pid)
{
  char buf[24];
  char *end;
  long value;
  int fd;
  ssize_t n;

  if (pid == NULL)
    {
      return -EINVAL;
    }

  fd = open(CONFIG_SYSTEM_TELNETD_PID_PATH, O_RDONLY);
  if (fd < 0)
    {
      return -errno;
    }

  n = read(fd, buf, sizeof(buf) - 1);
  close(fd);
  if (n <= 0)
    {
      return n < 0 ? -errno : -EINVAL;
    }

  buf[n] = '\0';
  value = strtol(buf, &end, 10);
  if (end == buf || value <= 0)
    {
      return -EINVAL;
    }

  *pid = (pid_t)value;
  return 0;
}

static int telnetd_stop(void)
{
  pid_t pid = 0;
  int ret;

  ret = telnetd_read_pid(&pid);
  if (ret < 0)
    {
      fprintf(stderr, "telnetd: no daemon PID at %s: %d\n",
              CONFIG_SYSTEM_TELNETD_PID_PATH, ret);
      return EXIT_FAILURE;
    }

  if (kill(pid, SIGTERM) < 0)
    {
      ret = -errno;
      if (ret == -ESRCH)
        {
          unlink(CONFIG_SYSTEM_TELNETD_PID_PATH);
        }

      fprintf(stderr, "telnetd: kill(%ld) failed: %d\n", (long)pid, ret);
      return EXIT_FAILURE;
    }

  unlink(CONFIG_SYSTEM_TELNETD_PID_PATH);
  printf("telnetd: stopped pid %ld\n", (long)pid);
  return EXIT_SUCCESS;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR char *argv_[] =
  {
    CONFIG_SYSTEM_TELNETD_PROGNAME,
    "-c",
    NULL,
  };

  struct telnetd_config_s config =
  {
    HTONS(CONFIG_SYSTEM_TELNETD_PORT),
#ifdef CONFIG_NET_IPv4
    AF_INET,
#else
    AF_INET6,
#endif
    CONFIG_SYSTEM_TELNETD_SESSION_PRIORITY,
    CONFIG_SYSTEM_TELNETD_SESSION_STACKSIZE,
#ifndef CONFIG_BUILD_KERNEL
    nsh_telnetmain,
#endif
#ifdef CONFIG_LIBC_EXECFUNCS
    CONFIG_SYSTEM_TELNETD_PROGNAME,
#endif
    argv_,
  };

  int daemon = 1;
  int stop = 0;
  int opt;

  while ((opt = getopt(argc, argv, "46ckp:")) != ERROR)
    {
      switch (opt)
        {
#ifdef CONFIG_NET_IPv4
          case '4':
            config.d_family = AF_INET;
            break;
#endif
#ifdef CONFIG_NET_IPv6
          case '6':
            config.d_family = AF_INET6;
            break;
#endif
          case 'c':
            daemon = 0;
            break;

          case 'k':
            stop = 1;
            break;

          case 'p':
            config.d_port = atoi(optarg);
            break;

          default:
            fprintf(stderr, "Usage: %s [-4|-6] [-c|-k] [-p port]\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }

  if (stop)
    {
      return telnetd_stop();
    }

  if (daemon)
    {
      int ret = telnetd_write_pid();

      if (ret < 0)
        {
          fprintf(stderr, "telnetd: warning: PID file %s failed: %d\n",
                  CONFIG_SYSTEM_TELNETD_PID_PATH, ret);
        }

      ret = telnetd_daemon(&config);
      unlink(CONFIG_SYSTEM_TELNETD_PID_PATH);
      return ret;
    }

  return nsh_telnetmain(1, argv);
}
