/****************************************************************************
 * apps/system/dvictrl/dvictrl_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef CONFIG_SYSTEM_DVICTRL_START_GUARD
#  include <arch/rp23xx/watchdog.h>
#  include <nuttx/timers/watchdog.h>
#endif

#include <nuttx/video/fb.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_SYSTEM_DVICTRL_DEVPATH
#  define CONFIG_SYSTEM_DVICTRL_DEVPATH "/dev/fb0"
#endif

#ifndef CONFIG_SYSTEM_DVICTRL_GUARD_DEVPATH
#  define CONFIG_SYSTEM_DVICTRL_GUARD_DEVPATH "/dev/watchdog0"
#endif

#ifndef CONFIG_SYSTEM_DVICTRL_GUARD_TIMEOUT_MS
#  define CONFIG_SYSTEM_DVICTRL_GUARD_TIMEOUT_MS 8000
#endif

#ifndef CONFIG_SYSTEM_DVICTRL_GUARD_SETTLE_MS
#  define CONFIG_SYSTEM_DVICTRL_GUARD_SETTLE_MS 3000
#endif

#define DVICTRL_BOOTGUARD_MAGIC 0x464a4247 /* "FJBG" */
#define DVICTRL_BOOTGUARD_STAGE_START 0x44564931 /* "DVI1" */
#define DVICTRL_USEC_PER_MSEC 1000u

#define RP23XX_DVIIOC_GETINFO _FBIOC(0x00f0)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct rp23xx_dvi_info_s
{
  uint32_t framebuffer_width;
  uint32_t framebuffer_height;
  uint32_t output_width;
  uint32_t output_height;
  uint32_t output_scaling;
  uint32_t bpp;
  uint32_t frame_bytes;
  uint32_t sys_clock;
  uint32_t peri_clock;
  uint32_t hstx_clock;
  uint32_t pixel_clock;
  uint32_t target_pixel_clock;
  uint32_t clkdiv;
  uint32_t scanout_mode;
  uint32_t power;
  uint32_t streaming;
  uint32_t dma_allocated;
  uint32_t pixel_ch;
  uint32_t command_ch;
  uint32_t irq_attached;
  uint32_t frame_count;
  uint32_t irq_count;
  uint32_t error_count;
  uint32_t hstx_csr;
  uint32_t hstx_fifo_stat;
  uint32_t dma_intr;
  uint32_t dma_inte2;
  uint32_t dma_ints2;
  uint32_t pixel_ctrl;
  uint32_t pixel_read_addr;
  uint32_t pixel_write_addr;
  uint32_t pixel_trans_count;
  uint32_t command_ctrl;
  uint32_t command_read_addr;
  uint32_t command_write_addr;
  uint32_t command_trans_count;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void dvictrl_usage(FAR const char *progname, int exitcode)
{
  fprintf(stderr,
          "Usage:\n"
          "  %s info\n"
          "  %s pattern colorbars\n"
          "  %s solid <rgb565>\n"
          "  %s start [--unguarded] [--guard-timeout <ms>]"
          " [--guard-window <ms>]\n"
          "  %s stop\n",
          progname, progname, progname, progname, progname);
  exit(exitcode);
}

static int dvictrl_open(void)
{
  int fd = open(CONFIG_SYSTEM_DVICTRL_DEVPATH, O_RDWR);

  if (fd < 0)
    {
      fprintf(stderr, "ERROR: open %s failed: %d\n",
              CONFIG_SYSTEM_DVICTRL_DEVPATH, errno);
    }

  return fd;
}

static int dvictrl_getinfo(int fd, FAR struct fb_videoinfo_s *vinfo,
                           FAR struct fb_planeinfo_s *pinfo)
{
  memset(vinfo, 0, sizeof(*vinfo));
  memset(pinfo, 0, sizeof(*pinfo));

  if (ioctl(fd, FBIOGET_VIDEOINFO, (unsigned long)((uintptr_t)vinfo)) < 0)
    {
      fprintf(stderr, "ERROR: FBIOGET_VIDEOINFO failed: %d\n", errno);
      return -errno;
    }

  pinfo->display = 0;
  if (ioctl(fd, FBIOGET_PLANEINFO, (unsigned long)((uintptr_t)pinfo)) < 0)
    {
      fprintf(stderr, "ERROR: FBIOGET_PLANEINFO failed: %d\n", errno);
      return -errno;
    }

  return 0;
}

static int dvictrl_print_info(int fd)
{
  struct fb_videoinfo_s vinfo;
  struct fb_planeinfo_s pinfo;
  struct rp23xx_dvi_info_s dinfo;
  int power;
  int ret;

  ret = dvictrl_getinfo(fd, &vinfo, &pinfo);
  if (ret < 0)
    {
      return ret;
    }

  power = -1;
  ret = ioctl(fd, FBIOGET_POWER, (unsigned long)((uintptr_t)&power));
  if (ret < 0)
    {
      fprintf(stderr, "WARNING: FBIOGET_POWER failed: %d\n", errno);
    }

  printf("device: %s\n", CONFIG_SYSTEM_DVICTRL_DEVPATH);
  printf("video: %ux%u fmt=%u planes=%u\n",
         vinfo.xres, vinfo.yres, vinfo.fmt, vinfo.nplanes);
  printf("plane: bpp=%u stride=%u fblen=%zu virt=%" PRIu32 "x%" PRIu32
         "\n",
         pinfo.bpp, pinfo.stride, pinfo.fblen,
         (uint32_t)pinfo.xres_virtual, (uint32_t)pinfo.yres_virtual);
  printf("power: %d\n", power);

  memset(&dinfo, 0, sizeof(dinfo));
  ret = ioctl(fd, RP23XX_DVIIOC_GETINFO,
              (unsigned long)((uintptr_t)&dinfo));
  if (ret < 0)
    {
      printf("dvi: debug-info unavailable\n");
      return 0;
    }

  printf("dvi: fb=%" PRIu32 "x%" PRIu32 " output=%" PRIu32 "x%" PRIu32
         " scale=%" PRIu32 " bpp=%" PRIu32 " bytes=%" PRIu32 "\n",
         dinfo.framebuffer_width, dinfo.framebuffer_height,
         dinfo.output_width, dinfo.output_height, dinfo.output_scaling,
         dinfo.bpp, dinfo.frame_bytes);
  printf("clock: sys=%" PRIu32 " peri=%" PRIu32 " hstx=%" PRIu32
         " target_pixel=%" PRIu32 " actual_pixel=%" PRIu32
         " clkdiv=%" PRIu32 "\n",
         dinfo.sys_clock, dinfo.peri_clock, dinfo.hstx_clock,
         dinfo.target_pixel_clock, dinfo.pixel_clock, dinfo.clkdiv);
  printf("scanout: %s\n",
         dinfo.scanout_mode == 1 ? "black-only" : "framebuffer");
  printf("dma: allocated=%" PRIu32 " pixel_ch=%" PRIu32
         " command_ch=%" PRIu32 " irq_attached=%" PRIu32
         " streaming=%" PRIu32 "\n",
         dinfo.dma_allocated, dinfo.pixel_ch, dinfo.command_ch,
         dinfo.irq_attached, dinfo.streaming);
  printf("frames: frame=%" PRIu32 " irq=%" PRIu32
         " errors=%" PRIu32 "\n",
         dinfo.frame_count, dinfo.irq_count, dinfo.error_count);
  printf("hstx: csr=0x%08" PRIx32 " fifo_stat=0x%08" PRIx32 "\n",
         dinfo.hstx_csr, dinfo.hstx_fifo_stat);
  printf("dmairq: intr=0x%08" PRIx32 " inte2=0x%08" PRIx32
         " ints2=0x%08" PRIx32 "\n",
         dinfo.dma_intr, dinfo.dma_inte2, dinfo.dma_ints2);
  printf("pixel: ctrl=0x%08" PRIx32 " read=0x%08" PRIx32
         " write=0x%08" PRIx32 " count=%" PRIu32 "\n",
         dinfo.pixel_ctrl, dinfo.pixel_read_addr, dinfo.pixel_write_addr,
         dinfo.pixel_trans_count);
  printf("command: ctrl=0x%08" PRIx32 " read=0x%08" PRIx32
         " write=0x%08" PRIx32 " count=%" PRIu32 "\n",
         dinfo.command_ctrl, dinfo.command_read_addr,
         dinfo.command_write_addr, dinfo.command_trans_count);

  return 0;
}

static int dvictrl_set_power(int fd, int power)
{
  if (ioctl(fd, FBIOSET_POWER, (unsigned long)power) < 0)
    {
      fprintf(stderr, "ERROR: FBIOSET_POWER(%d) failed: %d\n",
              power, errno);
      return -errno;
    }

  return 0;
}

#ifdef CONFIG_SYSTEM_DVICTRL_START_GUARD
static int dvictrl_guard_open(void)
{
  int fd = open(CONFIG_SYSTEM_DVICTRL_GUARD_DEVPATH, O_RDONLY);

  if (fd < 0)
    {
      fprintf(stderr, "ERROR: open %s failed: %d\n",
              CONFIG_SYSTEM_DVICTRL_GUARD_DEVPATH, errno);
    }

  return fd;
}

static int dvictrl_guard_ioctl(int fd, int cmd, unsigned long arg,
                               FAR const char *name)
{
  int ret;

  ret = ioctl(fd, cmd, arg);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: %s failed: %d\n", name, errno);
      return -errno;
    }

  return 0;
}

static int dvictrl_guard_arm(FAR int *guardfd, uint32_t timeout)
{
  int fd;
  int ret;

  fd = dvictrl_guard_open();
  if (fd < 0)
    {
      return -errno;
    }

  ret = dvictrl_guard_ioctl(fd, WDIOC_STOP, 0, "WDIOC_STOP");
  if (ret < 0)
    {
      goto errout;
    }

  ret = dvictrl_guard_ioctl(fd, WDIOC_SET_SCRATCH0,
                            DVICTRL_BOOTGUARD_MAGIC,
                            "WDIOC_SET_SCRATCH0");
  if (ret < 0)
    {
      goto errout;
    }

  ret = dvictrl_guard_ioctl(fd, WDIOC_SET_SCRATCH1, timeout,
                            "WDIOC_SET_SCRATCH1");
  if (ret < 0)
    {
      goto errout;
    }

  ret = dvictrl_guard_ioctl(fd, WDIOC_SET_SCRATCH2,
                            DVICTRL_BOOTGUARD_STAGE_START,
                            "WDIOC_SET_SCRATCH2");
  if (ret < 0)
    {
      goto errout;
    }

  ret = dvictrl_guard_ioctl(fd, WDIOC_SETTIMEOUT, timeout,
                            "WDIOC_SETTIMEOUT");
  if (ret < 0)
    {
      goto errout;
    }

  ret = dvictrl_guard_ioctl(fd, WDIOC_START, 0, "WDIOC_START");
  if (ret < 0)
    {
      goto errout;
    }

  *guardfd = fd;
  printf("bootguard: armed for %" PRIu32 " ms\n", timeout);
  return 0;

errout:
  close(fd);
  return ret;
}

static int dvictrl_guard_disarm(int fd)
{
  int ret = 0;

  if (fd < 0)
    {
      return 0;
    }

  if (dvictrl_guard_ioctl(fd, WDIOC_SET_SCRATCH0, 0,
                          "WDIOC_SET_SCRATCH0") < 0)
    {
      ret = -EIO;
    }

  if (dvictrl_guard_ioctl(fd, WDIOC_SET_SCRATCH1, 0,
                          "WDIOC_SET_SCRATCH1") < 0)
    {
      ret = -EIO;
    }

  if (dvictrl_guard_ioctl(fd, WDIOC_SET_SCRATCH2, 0,
                          "WDIOC_SET_SCRATCH2") < 0)
    {
      ret = -EIO;
    }

  if (dvictrl_guard_ioctl(fd, WDIOC_STOP, 0, "WDIOC_STOP") < 0)
    {
      ret = -EIO;
    }

  printf("bootguard: disarmed\n");
  return ret;
}

static int dvictrl_parse_u32(FAR const char *arg, FAR uint32_t *value)
{
  FAR char *endptr;
  unsigned long parsed;

  errno = 0;
  parsed = strtoul(arg, &endptr, 0);
  if (errno != 0 || *endptr != '\0' || parsed > UINT32_MAX)
    {
      return -EINVAL;
    }

  *value = parsed;
  return 0;
}

static int dvictrl_start(int fd, int argc, FAR char *argv[])
{
  uint32_t timeout = CONFIG_SYSTEM_DVICTRL_GUARD_TIMEOUT_MS;
  uint32_t settle = CONFIG_SYSTEM_DVICTRL_GUARD_SETTLE_MS;
  bool guarded = true;
  int guardfd = -1;
  int ret;
  int i;

  for (i = 2; i < argc; i++)
    {
      if (strcmp(argv[i], "--unguarded") == 0 ||
          strcmp(argv[i], "unguarded") == 0)
        {
          guarded = false;
        }
      else if (strcmp(argv[i], "--guard-timeout") == 0)
        {
          if (++i >= argc || dvictrl_parse_u32(argv[i], &timeout) < 0 ||
              timeout == 0)
            {
              dvictrl_usage(argv[0], EXIT_FAILURE);
            }
        }
      else if (strcmp(argv[i], "--guard-window") == 0)
        {
          if (++i >= argc || dvictrl_parse_u32(argv[i], &settle) < 0)
            {
              dvictrl_usage(argv[0], EXIT_FAILURE);
            }
        }
      else
        {
          dvictrl_usage(argv[0], EXIT_FAILURE);
        }
    }

  if (guarded)
    {
      if (timeout <= settle)
        {
          fprintf(stderr,
                  "ERROR: guard timeout must be greater than guard window\n");
          return -EINVAL;
        }

      ret = dvictrl_guard_arm(&guardfd, timeout);
      if (ret < 0)
        {
          fprintf(stderr,
                  "ERROR: DVI start guard unavailable; use --unguarded"
                  " only for hands-on tests\n");
          return ret;
        }
    }

  ret = dvictrl_set_power(fd, 1);
  if (ret < 0)
    {
      if (guarded)
        {
          dvictrl_guard_disarm(guardfd);
          close(guardfd);
        }

      return ret;
    }

  if (guarded && settle > 0)
    {
      printf("bootguard: holding for %" PRIu32 " ms\n", settle);
      usleep(settle * DVICTRL_USEC_PER_MSEC);
    }

  if (guarded)
    {
      ret = dvictrl_guard_disarm(guardfd);
      close(guardfd);
      if (ret < 0)
        {
          return ret;
        }
    }

  return 0;
}
#else
static int dvictrl_start(int fd, int argc, FAR char *argv[])
{
  int i;

  for (i = 2; i < argc; i++)
    {
      if (strcmp(argv[i], "--unguarded") != 0 &&
          strcmp(argv[i], "unguarded") != 0)
        {
          dvictrl_usage(argv[0], EXIT_FAILURE);
        }
    }

  return dvictrl_set_power(fd, 1);
}
#endif

static int dvictrl_map_framebuffer(int fd,
                                   FAR struct fb_videoinfo_s *vinfo,
                                   FAR struct fb_planeinfo_s *pinfo,
                                   FAR uint8_t **fbmem)
{
  FAR void *map;
  int ret;

  ret = dvictrl_getinfo(fd, vinfo, pinfo);
  if (ret < 0)
    {
      return ret;
    }

  if (vinfo->fmt != FB_FMT_RGB16_565 || pinfo->bpp != 16)
    {
      fprintf(stderr, "ERROR: unsupported framebuffer format/bpp: %u/%u\n",
              vinfo->fmt, pinfo->bpp);
      return -EINVAL;
    }

  map = mmap(NULL, pinfo->fblen, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (map == MAP_FAILED)
    {
      fprintf(stderr, "ERROR: mmap failed: %d\n", errno);
      return -errno;
    }

  *fbmem = map;
  return 0;
}

static void dvictrl_fill_solid(FAR uint8_t *fbmem,
                               FAR const struct fb_videoinfo_s *vinfo,
                               FAR const struct fb_planeinfo_s *pinfo,
                               uint16_t color)
{
  unsigned int x;
  unsigned int y;

  for (y = 0; y < vinfo->yres; y++)
    {
      FAR uint16_t *row = (FAR uint16_t *)(fbmem + y * pinfo->stride);

      for (x = 0; x < vinfo->xres; x++)
        {
          row[x] = color;
        }
    }
}

static void dvictrl_fill_colorbars(FAR uint8_t *fbmem,
                                   FAR const struct fb_videoinfo_s *vinfo,
                                   FAR const struct fb_planeinfo_s *pinfo)
{
  static const uint16_t colors[] =
    {
      0xffff, 0xffe0, 0x07ff, 0x07e0,
      0xf81f, 0xf800, 0x001f, 0x0000
    };
  unsigned int bar_width = MAX(1, vinfo->xres / nitems(colors));
  unsigned int x;
  unsigned int y;

  for (y = 0; y < vinfo->yres; y++)
    {
      FAR uint16_t *row = (FAR uint16_t *)(fbmem + y * pinfo->stride);

      for (x = 0; x < vinfo->xres; x++)
        {
          unsigned int bar = x / bar_width;

          if (bar >= nitems(colors))
            {
              bar = nitems(colors) - 1;
            }

          row[x] = colors[bar];
        }
    }
}

static int dvictrl_pattern_colorbars(int fd)
{
  struct fb_videoinfo_s vinfo;
  struct fb_planeinfo_s pinfo;
  FAR uint8_t *fbmem;
  int ret;

  ret = dvictrl_map_framebuffer(fd, &vinfo, &pinfo, &fbmem);
  if (ret < 0)
    {
      return ret;
    }

  dvictrl_fill_colorbars(fbmem, &vinfo, &pinfo);
  munmap(fbmem, pinfo.fblen);

  printf("filled colorbars\n");
  return 0;
}

static int dvictrl_solid(int fd, FAR const char *arg)
{
  struct fb_videoinfo_s vinfo;
  struct fb_planeinfo_s pinfo;
  FAR uint8_t *fbmem;
  FAR char *endptr;
  unsigned long value;
  int ret;

  errno = 0;
  value = strtoul(arg, &endptr, 0);
  if (errno != 0 || *endptr != '\0' || value > 0xffff)
    {
      fprintf(stderr, "ERROR: invalid RGB565 color: %s\n", arg);
      return -EINVAL;
    }

  ret = dvictrl_map_framebuffer(fd, &vinfo, &pinfo, &fbmem);
  if (ret < 0)
    {
      return ret;
    }

  dvictrl_fill_solid(fbmem, &vinfo, &pinfo, (uint16_t)value);
  munmap(fbmem, pinfo.fblen);

  printf("filled solid 0x%04lx\n", value);
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int fd;
  int ret;

  if (argc < 2)
    {
      dvictrl_usage(argv[0], EXIT_FAILURE);
    }

  fd = dvictrl_open();
  if (fd < 0)
    {
      return EXIT_FAILURE;
    }

  if (strcmp(argv[1], "info") == 0)
    {
      ret = dvictrl_print_info(fd);
    }
  else if (strcmp(argv[1], "start") == 0)
    {
      ret = dvictrl_start(fd, argc, argv);
    }
  else if (strcmp(argv[1], "stop") == 0)
    {
      ret = dvictrl_set_power(fd, 0);
    }
  else if (strcmp(argv[1], "pattern") == 0)
    {
      if (argc != 3 || strcmp(argv[2], "colorbars") != 0)
        {
          close(fd);
          dvictrl_usage(argv[0], EXIT_FAILURE);
        }

      ret = dvictrl_pattern_colorbars(fd);
    }
  else if (strcmp(argv[1], "solid") == 0)
    {
      if (argc != 3)
        {
          close(fd);
          dvictrl_usage(argv[0], EXIT_FAILURE);
        }

      ret = dvictrl_solid(fd, argv[2]);
    }
  else
    {
      close(fd);
      dvictrl_usage(argv[0], EXIT_FAILURE);
    }

  close(fd);
  return ret < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
