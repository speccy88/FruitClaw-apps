/****************************************************************************
 * apps/system/readline/readline_common.c
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

#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <nuttx/debug.h>

#include <nuttx/ascii.h>
#include <nuttx/vt100.h>
#include <nuttx/lib/builtin.h>

#include "system/readline.h"
#include "readline.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_READLINE_CMD_HISTORY
#  define RL_CMDHIST_LEN        CONFIG_READLINE_CMD_HISTORY_LEN
#  define RL_CMDHIST_LINELEN    CONFIG_READLINE_CMD_HISTORY_LINELEN
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

#ifdef CONFIG_READLINE_CMD_HISTORY
struct cmdhist_s
{
  char buf[RL_CMDHIST_LEN][RL_CMDHIST_LINELEN];  /* Circular buffer */
  int  head;                                     /* Head of the circular buffer */
  int  offset;                                   /* Offset from head */
  int  len;                                      /* Size of the circular buffer */
};
#endif /* CONFIG_READLINE_CMD_HISTORY */

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* <esc>[K is the VT100 command erases to the end of the line. */

#ifdef CONFIG_READLINE_ECHO
static const char g_erasetoeol[] = VT100_CLEAREOL;
static const char g_cursorleft[] = VT100_CURSORLF('1');
static const char g_cursorright[] = VT100_CURSORRT('1');
#endif

#ifdef CONFIG_READLINE_TABCOMPLETION
/* Prompt string to present at the beginning of the line */

static FAR const char *g_readline_prompt = NULL;

#ifdef CONFIG_READLINE_HAVE_EXTMATCH
static FAR const struct extmatch_vtable_s *g_extmatch_vtbl = NULL;
#endif
#endif /* CONFIG_READLINE_TABCOMPLETION */

#ifdef CONFIG_READLINE_CMD_HISTORY
static struct cmdhist_s g_cmdhist;
#endif /* CONFIG_READLINE_CMD_HISTORY */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: move_cursor_left
 ****************************************************************************/

#ifdef CONFIG_READLINE_ECHO
static void move_cursor_left(FAR struct rl_common_s *vtbl, int count)
{
  while (count-- > 0)
    {
      RL_WRITE(vtbl, g_cursorleft, sizeof(g_cursorleft));
    }
}
#endif

/****************************************************************************
 * Name: move_cursor_right
 ****************************************************************************/

#ifdef CONFIG_READLINE_ECHO
static void move_cursor_right(FAR struct rl_common_s *vtbl, int count)
{
  while (count-- > 0)
    {
      RL_WRITE(vtbl, g_cursorright, sizeof(g_cursorright));
    }
}
#endif

/****************************************************************************
 * Name: redraw_from_cursor
 ****************************************************************************/

#ifdef CONFIG_READLINE_ECHO
static void redraw_from_cursor(FAR struct rl_common_s *vtbl,
                               FAR const char *buf, int nch, int cursor)
{
  int tail = nch - cursor;

  if (tail > 0)
    {
      RL_WRITE(vtbl, &buf[cursor], tail);
    }

  RL_WRITE(vtbl, g_erasetoeol, sizeof(g_erasetoeol));
  move_cursor_left(vtbl, tail);
}
#endif

/****************************************************************************
 * Name: clear_input_line
 ****************************************************************************/

#ifdef CONFIG_READLINE_ECHO
static void clear_input_line(FAR struct rl_common_s *vtbl, int cursor)
{
  move_cursor_left(vtbl, cursor);
  RL_WRITE(vtbl, g_erasetoeol, sizeof(g_erasetoeol));
}
#endif

/****************************************************************************
 * Name: count_builtin_matches
 *
 * Description:
 *   Count the number of builtin commands
 *
 * Input Parameters:
 *   matches - Array to save builtin command index.
 *   len - The length of the matching name to try
 *
 * Returned Value:
 *   The number of matching names
 *
 ****************************************************************************/

#if defined(CONFIG_READLINE_TABCOMPLETION) && defined(CONFIG_BUILTIN)
static int count_builtin_matches(FAR char *buf, FAR int *matches,
                                 int namelen)
{
#if CONFIG_READLINE_MAX_BUILTINS > 0
  FAR const char *name;
  int nr_matches = 0;
  int i;

  for (i = 0; (name = builtin_getname(i)) != NULL; i++)
    {
      if (strncmp(buf, name, namelen) == 0)
        {
          matches[nr_matches] = i;
          nr_matches++;

          if (nr_matches >= CONFIG_READLINE_MAX_BUILTINS)
            {
              break;
            }
        }
    }

  return nr_matches;

#else
  return 0;
#endif
}
#endif

/****************************************************************************
 * Name: tab_completion
 *
 * Description:
 *   Unix like tab completion, only for builtin apps
 *
 * Input Parameters:
 *   vtbl   - vtbl used to access implementation specific interface
 *   buf     - The user allocated buffer to be filled.
 *   buflen  - the size of the buffer.
 *   nch     - the number of characters.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

#ifdef CONFIG_READLINE_TABCOMPLETION
static void tab_completion(FAR struct rl_common_s *vtbl, char *buf,
                           int buflen, int *nch)
{
  FAR const char *name = NULL;
  char tmp_name[CONFIG_TASK_NAME_SIZE + 1];
#ifdef CONFIG_BUILTIN
  int nr_builtin_matches = 0;
  int builtin_matches[CONFIG_READLINE_MAX_BUILTINS];
#endif
#ifdef CONFIG_READLINE_HAVE_EXTMATCH
  int nr_ext_matches = 0;
  int ext_matches[CONFIG_READLINE_MAX_EXTCMDS];
#endif
  int nr_matches;
  int len = *nch;
  int name_len;
  int i;
  int j;

  if (len >= 1)
    {
#ifdef CONFIG_BUILTIN
      /* Count the matching builtin commands */

      nr_builtin_matches = count_builtin_matches(buf, builtin_matches, len);
      nr_matches         = nr_builtin_matches;
#else
      nr_matches         = 0;
#endif

#ifdef CONFIG_READLINE_HAVE_EXTMATCH
      /* Is there registered external handling logic? */

      nr_ext_matches     = 0;
      if (g_extmatch_vtbl != NULL)
        {
          /* Count the number of external commands */

          nr_ext_matches =
            g_extmatch_vtbl->count_matches(buf, ext_matches, len);
          nr_matches    += nr_ext_matches;
        }

#endif

      /* Is there only one matching name? */

      if (nr_matches == 1)
        {
          /* Yes... that that is the one we want.  Was it a match with a
           * builtin command?  Or with an external command.
           */

#ifdef CONFIG_BUILTIN
#ifdef CONFIG_READLINE_HAVE_EXTMATCH
          if (nr_builtin_matches ==  1)
#endif
            {
              /* It is a match with a builtin command */

              name = builtin_getname(builtin_matches[0]);
            }
#endif

#ifdef CONFIG_READLINE_HAVE_EXTMATCH
#ifdef CONFIG_BUILTIN
          else
#endif
            {
              /* It is a match with an external command */

              name = g_extmatch_vtbl->getname(ext_matches[0]);
            }
#endif

          /* Copy the name to the command buffer and to the display. */

          name_len = strlen(name);

          for (j = len; j < name_len; j++)
            {
              buf[j] = name[j];
              RL_PUTC(vtbl, name[j]);
            }

          /* Don't remove extra characters after the completed word,
           * if any.
           */

          if (len < name_len)
            {
              *nch = name_len;
            }
        }

      /* Are there multiple matching names? */

      else if (nr_matches > 1)
        {
          RL_PUTC(vtbl, '\n');

          /* See how many characters we can auto complete for the user
           * For example, if we have the following commands:
           * - prog1
           * - prog2
           * - prog3
           * then it should automatically complete up to prog.
           * We do this in one pass using a temp.
           */

          memset(tmp_name, 0, sizeof(tmp_name));

#ifdef CONFIG_READLINE_HAVE_EXTMATCH
          /* Show the possible external completions */

          for (i = 0; i < nr_ext_matches; i++)
            {
              name = g_extmatch_vtbl->getname(ext_matches[i]);

              /* Initialize temp */

              if (tmp_name[0] == '\0')
                {
                  strlcpy(tmp_name, name, sizeof(tmp_name));
                }

              RL_PUTC(vtbl, ' ');
              RL_PUTC(vtbl, ' ');

              for (j = 0; j < (int)strlen(name); j++)
                {
                  /* Removing characters that aren't common to all the
                   * matches.
                   */

                  if (j < (int)sizeof(tmp_name) && name[j] != tmp_name[j])
                    {
                      tmp_name[j] = '\0';
                    }

                  RL_PUTC(vtbl, name[j]);
                }

              RL_PUTC(vtbl, '\n');
            }
#endif

#ifdef CONFIG_BUILTIN
          /* Show the possible builtin completions */

          for (i = 0; i < nr_builtin_matches; i++)
            {
              name = builtin_getname(builtin_matches[i]);

              /* Initialize temp */

              if (tmp_name[0] == '\0')
                {
                  strlcpy(tmp_name, name, sizeof(tmp_name));
                }

              RL_PUTC(vtbl, ' ');
              RL_PUTC(vtbl, ' ');

              for (j = 0; j < strlen(name); j++)
                {
                  /* Removing characters that aren't common to all the
                   * matches.
                   */

                  if (j < sizeof(tmp_name) && name[j] != tmp_name[j])
                    {
                      tmp_name[j] = '\0';
                    }

                  RL_PUTC(vtbl, name[j]);
                }

              RL_PUTC(vtbl, '\n');
            }

#endif
          strlcpy(buf, tmp_name, buflen);
          name_len = strlen(tmp_name);

          /* Output the original prompt */

          if (g_readline_prompt != NULL)
            {
              for (i = 0; i < (int)strlen(g_readline_prompt); i++)
                {
                  RL_PUTC(vtbl, g_readline_prompt[i]);
                }
            }

          for (i = 0; i < name_len; i++)
            {
              RL_PUTC(vtbl, buf[i]);
            }

          /* Don't remove extra characters after the completed word,
           * if any
           */

          if (len < name_len)
            {
              *nch = name_len;
            }
        }
    }
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: readline_prompt
 *
 *   If a prompt string is used by the application, then the application
 *   must provide the prompt string to readline() by calling this function.
 *   This is needed only for tab completion in cases where is it necessary
 *   to reprint the prompt string.
 *
 * Input Parameters:
 *   prompt    - The prompt string. This function may then be
 *   called with that value in order to restore the previous vtable.
 *
 * Returned values:
 *   Returns the previous value of the prompt string.  This function may
 *   then be called with that value in order to restore the previous prompt.
 *
 * Assumptions:
 *   The prompt string is statically allocated a global.  readline() will
 *   simply remember the pointer to the string.  The string must stay
 *   allocated and available.  Only one prompt string is supported.  If
 *   there are multiple clients of readline(), they must all share the same
 *   prompt string (with exceptions in the case of the kernel build).
 *
 ****************************************************************************/

#ifdef CONFIG_READLINE_TABCOMPLETION
FAR const char *readline_prompt(FAR const char *prompt)
{
  FAR const char *ret = g_readline_prompt;
  g_readline_prompt = prompt;
  return ret;
}
#endif

/****************************************************************************
 * Name: readline_extmatch
 *
 *   If the applications supports a command set, then it may call this
 *   function in order to provide support for tab complete on these
 *   "external"  commands
 *
 * Input Parameters:
 *   vtbl - Callbacks to access the external names.
 *
 * Returned values:
 *   Returns the previous vtable pointer.  This function may then be
 *   called with that value in order to restore the previous vtable.
 *
 * Assumptions:
 *   The vtbl string is statically allocated a global.  readline() will
 *   simply remember the pointer to the structure.  The structure must stay
 *   allocated and available.  Only one instance of such a structure is
 *   supported.  If there are multiple clients of readline(), they must all
 *   share the same tab-completion logic (with exceptions in the case of
 *   the kernel build).
 *
 ****************************************************************************/

#if defined(CONFIG_READLINE_TABCOMPLETION) && defined(CONFIG_READLINE_HAVE_EXTMATCH)
FAR const struct extmatch_vtable_s *
  readline_extmatch(FAR const struct extmatch_vtable_s *vtbl)
{
#if (CONFIG_READLINE_MAX_EXTCMDS > 0)
  FAR const struct extmatch_vtable_s *ret = g_extmatch_vtbl;
  g_extmatch_vtbl = vtbl;
  return ret;
#else
  return NULL;
#endif
}
#endif

/****************************************************************************
 * Name: readline_common
 *
 * Description:
 *   readline() reads in at most one less than 'buflen' characters from
 *   'instream' and stores them into the buffer pointed to by 'buf'.
 *   Characters are echoed on 'outstream'.  Reading stops after an EOF or a
 *   newline.  If a newline is read, it is stored into the buffer.  A null
 *   terminator is stored after the last character in the buffer.
 *
 *   This version of realine assumes that we are reading and writing to
 *   a VT100 console.  This will not work well if 'instream' or 'outstream'
 *   corresponds to a raw byte steam.
 *
 *   This function is inspired by the GNU readline but is an entirely
 *   different creature.
 *
 * Input Parameters:
 *   vtbl    - vtbl used to access implementation specific interface
 *   buf     - The user allocated buffer to be filled.
 *   buflen  - the size of the buffer.
 *
 * Returned Value:
 *   On success, the (positive) number of bytes transferred is returned.
 *   EOF is returned to indicate either an end of file condition or a
 *   failure.
 *
 ****************************************************************************/

ssize_t readline_common(FAR struct rl_common_s *vtbl, FAR char *buf,
                        int buflen)
{
  int escape;
  int cursor;
  int nch;
#ifdef CONFIG_READLINE_CMD_HISTORY
  int i;
#endif

  /* Sanity checks */

  DEBUGASSERT(buf && buflen > 0);

  if (buflen < 2)
    {
      *buf = '\0';
      return 0;
    }

  /* <esc>[K is the VT100 command that erases to the end of the line. */

#ifdef CONFIG_READLINE_ECHO
  RL_WRITE(vtbl, g_erasetoeol, sizeof(g_erasetoeol));
#endif

  /* Read characters until we have a full line. On each the loop we must
   * be assured that there are two free bytes in the line buffer:  One for
   * the next character and one for the null terminator.
   */

  escape = 0;
  cursor = 0;
  nch    = 0;

  for (; ; )
    {
      /* Get the next character. readline_rawgetc() returns EOF on any
       * errors or at the end of file.
       */

      int ch = RL_GETC(vtbl);

      /* Check for end-of-file or read error */

      if (ch == EOF)
        {
          /* Did we already received some data? */

          if (nch > 0)
            {
              /* Yes.. Terminate the line (which might be zero length)
               * and return the data that was received.  The end-of-file
               * or error condition will be reported next time.
               */

              buf[nch] = '\0';
              return nch;
            }

          return EOF;
        }

      /* Are we processing a VT100 escape sequence */

      else if (escape)
        {
          /* Yes.  Track CSI arrow/editing sequences. */

          if (escape == 1)
            {
              if (ch == ASCII_LBRACKET)
                {
                  escape = 2;
                }
              else
                {
                  escape = 0;
                }

              continue;
            }

          if (escape == 2 && ch >= ASCII_0 && ch <= ASCII_9)
            {
              escape = ch;
              continue;
            }

          if (escape >= ASCII_0 && escape <= ASCII_9)
            {
              int parameter = escape;

              escape = 0;
              if (ch == '~')
                {
                  switch (parameter)
                    {
                      case ASCII_1:
                      case ASCII_7:
#ifdef CONFIG_READLINE_ECHO
                        move_cursor_left(vtbl, cursor);
#endif
                        cursor = 0;
                        break;

                      case ASCII_3:
                        if (cursor < nch)
                          {
                            int tail = nch - cursor - 1;

                            if (tail > 0)
                              {
                                memmove(&buf[cursor], &buf[cursor + 1],
                                        tail);
                              }

                            nch--;
                            buf[nch] = '\0';

#ifdef CONFIG_READLINE_ECHO
                            redraw_from_cursor(vtbl, buf, nch, cursor);
#endif
                          }
                        break;

                      case ASCII_4:
                      case ASCII_8:
#ifdef CONFIG_READLINE_ECHO
                        move_cursor_right(vtbl, nch - cursor);
#endif
                        cursor = nch;
                        break;
                    }
                }

              continue;
            }

#ifdef CONFIG_READLINE_CMD_HISTORY
          /* Intercept up and down arrow keys */

          if ((ch == 'A' || ch == 'B') && g_cmdhist.len > 0)
            {
              if (ch == 'A') /* up arrow */
                {
                  /* Go to the past command in history */

                  g_cmdhist.offset--;

                  if (-g_cmdhist.offset >= g_cmdhist.len)
                    {
                      g_cmdhist.offset = -(g_cmdhist.len - 1);
                    }
                }
              else /* down arrow */
                {
                  /* Go to the recent command */

                  g_cmdhist.offset++;

                  if (g_cmdhist.offset > 1)
                    {
                      g_cmdhist.offset = 1;
                    }
                }

#ifdef CONFIG_READLINE_ECHO
              clear_input_line(vtbl, cursor);
#endif
              nch    = 0;
              cursor = 0;

              if (g_cmdhist.offset != 1)
                {
                  int idx = g_cmdhist.head + g_cmdhist.offset;

                  /* Circular buffer wrap around */

                  if (idx < 0)
                    {
                      idx = idx + RL_CMDHIST_LEN;
                    }
                  else if (idx >= RL_CMDHIST_LEN)
                    {
                      idx = idx - RL_CMDHIST_LEN;
                    }

                  for (i = 0; g_cmdhist.buf[idx][i] != '\0'; i++)
                    {
                      if (nch + 1 >= buflen)
                        {
                          break;
                        }

                      buf[nch++] = g_cmdhist.buf[idx][i];
                    }

                  cursor = nch;
#ifdef CONFIG_READLINE_ECHO
                  RL_WRITE(vtbl, buf, nch);
#endif
                }

              buf[nch] = '\0';
              escape = 0;
              continue;
            }
#endif /* CONFIG_READLINE_CMD_HISTORY */

          if (ch == 'C') /* right arrow */
            {
              if (cursor < nch)
                {
#ifdef CONFIG_READLINE_ECHO
                  move_cursor_right(vtbl, 1);
#endif
                  cursor++;
                }
            }
          else if (ch == 'D') /* left arrow */
            {
              if (cursor > 0)
                {
#ifdef CONFIG_READLINE_ECHO
                  move_cursor_left(vtbl, 1);
#endif
                  cursor--;
                }
            }
          else if (ch == 'H') /* home */
            {
#ifdef CONFIG_READLINE_ECHO
              move_cursor_left(vtbl, cursor);
#endif
              cursor = 0;
            }
          else if (ch == 'F') /* end */
            {
#ifdef CONFIG_READLINE_ECHO
              move_cursor_right(vtbl, nch - cursor);
#endif
              cursor = nch;
            }

          escape = 0;
          continue;
        }

      /* Check for backspace
       *
       * There are several notions of backspace, for an elaborate summary see
       * http://www.ibb.net/~anne/keyboard.html. There is no clean solution.
       * Here both DEL and backspace are treated like backspace here.  The
       * Unix/Linux screen terminal by default outputs  DEL (0x7f) when the
       * backspace key is pressed.
       */

      else if (ch == ASCII_BS || ch == ASCII_DEL)
        {
          /* Eliminate the character to the left of the cursor. */

          if (cursor > 0)
            {
              int tail;

              cursor--;
              tail = nch - cursor - 1;
              if (tail > 0)
                {
                  memmove(&buf[cursor], &buf[cursor + 1], tail);
                }

              nch--;
              buf[nch] = '\0';

#ifdef CONFIG_READLINE_ECHO
              /* Echo the backspace character on the console.  Always output
               * the backspace character because the VT100 terminal doesn't
               * understand DEL properly.
               */

              RL_PUTC(vtbl, ASCII_BS);
              redraw_from_cursor(vtbl, buf, nch, cursor);
#endif
            }
        }

      /* Check for the beginning of a VT100 escape sequence */

      else if (ch == ASCII_ESC)
        {
          /* The next character is escaped */

          escape = 1;
        }

      /* Check for end-of-line.  This is tricky only in that some
       * environments may return CR as end-of-line, others LF, and
       * others both.
       */

      else if (ch == '\n' || ch == '\r')
        {
#ifdef CONFIG_READLINE_CMD_HISTORY
          /* Save history of command, only if there was something
           * typed besides return character.
           */

          if (nch >= 1)
            {
              /* If this command is the one at the top of the circular
               * buffer, don't save it again.
               */

              if (strncmp(buf, g_cmdhist.buf[g_cmdhist.head], nch + 1) != 0)
                {
                  g_cmdhist.head = (g_cmdhist.head + 1) % RL_CMDHIST_LEN;

                  for (i = 0; (i < nch) && i < (RL_CMDHIST_LINELEN - 1); i++)
                    {
                      g_cmdhist.buf[g_cmdhist.head][i] = buf[i];
                    }

                  g_cmdhist.buf[g_cmdhist.head][i] = '\0';

                  if (g_cmdhist.len < RL_CMDHIST_LEN)
                    {
                      g_cmdhist.len++;
                    }
                }

              g_cmdhist.offset = 1;
            }
#endif /* CONFIG_READLINE_CMD_HISTORY */

          /* The newline is stored in the buffer along with the null
           * terminator.
           */

          buf[nch++] = '\n';
          buf[nch]   = '\0';

          return nch;
        }

      /* Otherwise, put the character in the line buffer if the
       * character is not a control byte
       */

      else if (!iscntrl(ch & 0xff))
        {
          /* Check if there is room for another character and the line's
           * null terminator.  If not then we have to end the line now.
           */

          if (nch + 1 >= buflen)
            {
              buf[nch] = '\0';
              return nch;
            }

          if (cursor < nch)
            {
              memmove(&buf[cursor + 1], &buf[cursor], nch - cursor);
              buf[cursor] = ch;
              nch++;
              cursor++;
              buf[nch] = '\0';

#ifdef CONFIG_READLINE_ECHO
              RL_PUTC(vtbl, ch);
              redraw_from_cursor(vtbl, buf, nch, cursor);
#endif
            }
          else
            {
              buf[nch++] = ch;
              cursor = nch;
              buf[nch] = '\0';

#ifdef CONFIG_READLINE_ECHO
              RL_PUTC(vtbl, ch);
#endif
            }
        }
#ifdef CONFIG_READLINE_TABCOMPLETION
      else if (ch == '\t') /* TAB character */
        {
          if (cursor == nch)
            {
              tab_completion(vtbl, buf, buflen, &nch);
              cursor = nch;
            }
        }
#endif
    }
}
