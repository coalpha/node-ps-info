#include "lib.h"

#define _POSIX_C_SOURCE 200809L
#include <stdlib.h> // malloc
#include <stdint.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h> // PATH_MAX
#include <fcntl.h>  // openat

#define LEN(s) sizeof(s) - 1
#define S_PROC_PATH "/proc/"
#define S_MAX_PID   "4194304"
#ifndef TASK_COMM_LEN
#define TASK_COMM_LEN 16
#define S_I32_MAX   "2147483647"
#define S_U32_MAX   "4294967295"
#define S_I64_MAX   "9223372036854775807"
#define S_U64_MAX   "18446744073709551615"
#endif
static char proc_path[LEN(S_PROC_PATH) + LEN(S_MAX_PID) + 1] = "/proc/";

char *path_buf;

napi_value init_all(napi_env const env, napi_value exports) {
   path_buf = malloc(PATH_MAX);

   if (path_buf == NULL) {
      napi_throw_error(env, "ENOMEM", "Allocating PATH_MAX bytes failed!");
      return NULL;
   }

   if (napi_create_function(env, "ps_list", sizeof("ps_list"), ps_list, NULL, &exports) != napi_ok) {
      napi_throw_error(env, "ENOCREATE", "Could not export ps_list!");
      return NULL;
   }

   return exports;
}

napi_value ps_list(napi_env const restrict env, napi_callback_info const restrict info) {
   (void) info;

   napi_value ary;
   if (napi_create_array(env, &ary) != napi_ok) {
      napi_throw_error(env, "ENOCREATE", "Could not make output array!");
      return NULL;
   }

   DIR *const restrict proc = opendir(proc_path);
   if (proc == NULL) {
      napi_throw_error(env, "EACCESS", "Could not open /proc/!");
      return NULL;
   }

   size_t ary_idx = 0;
   napi_value obj;
   napi_value val;

   struct dirent *restrict dirent;
   while ((dirent = readdir(proc))) {
      if (napi_create_object(env, &obj) != napi_ok) {
         return ary;
      }

      // pid
      {
         uint32_t pid = 0;

         // simultaniously check if the directory is a PID, copy it into the
         // proc_path, and parse it's name as a u32.
         char *c = dirent->d_name;
         char *proc_path6 = proc_path + 6;
         while (*c) {
            if (*c < '0' ||'9' < *c) {
               goto next_dirent;
            }

            pid *= 10;
            pid += *c - '0';
            *proc_path6++ = *c++;
         }
         *proc_path6 = '\0';

         if (napi_create_uint32(env, pid, &val) != napi_ok) {
            return ary;
         }

         if (napi_set_named_property(env, obj, "id", val) != napi_ok) {
            return ary;
         }
      }

      DIR *const restrict cur_dir = opendir(proc_path);

      if (cur_dir == NULL) {
         goto next_dirent;
      }

      int const cur_dirfd = dirfd(cur_dir);

      int const statfd = openat(cur_dirfd, "stat", O_RDONLY);
      if (statfd < 0) {
         goto die_cur_dir;
      }

      char stat_buf[0
         + LEN(S_MAX_PID)  // 01 pid
         + LEN(" ")
         + LEN("(")
         + TASK_COMM_LEN   // 02 comm
         + LEN(")")
         + LEN(" ")
         + LEN("?")        // 03 state
         + LEN(" ")
         + LEN(S_MAX_PID)  // 04 ppid
         + LEN(" ")
         + LEN(S_MAX_PID)  // 05 pgid
         + LEN(" ")
         + LEN(S_I32_MAX)  // 06 session id
         + LEN(" ")
         + LEN(S_I32_MAX)  // 07 tty_nr
         + LEN(" ")
         + LEN(S_I32_MAX)  // 08 tpgid
         + LEN(" ")
         + LEN(S_U32_MAX)  // 09 flags
         + LEN(" ")
         + LEN(S_U32_MAX)  // 10 minflt
         + LEN(" ")
         + LEN(S_U32_MAX)  // 11 cminflt
         + LEN(" ")
         + LEN(S_U32_MAX)  // 12 majflt
         + LEN(" ")
         + LEN(S_U32_MAX)  // 13 cmajflt
         + LEN(" ")
         + LEN(S_U32_MAX)  // 14 utime
         + LEN(" ")
         + LEN(S_U32_MAX)  // 15 stime
         + LEN(" ")
         + LEN(S_I32_MAX)  // 16 cutime
         + LEN(" ")
         + LEN(S_I32_MAX)  // 17 cstime
         + LEN(" ")
         + LEN(S_I32_MAX)  // 18 priority
         + LEN(" ")
         + LEN(S_I32_MAX)  // 19 nice
         + LEN(" ")
         + LEN(S_I32_MAX)  // 20 num_threads
      ];

      char *stat_buf_end;
      {
         ssize_t bytes_read = read(statfd, stat_buf, sizeof(stat_buf) - 1);
         if (bytes_read == -1) {
            goto die_statfd;
         }
         stat_buf_end = stat_buf + bytes_read;
      }

      // manually inlined function
      // to invoke, set delim and return_address
      // result is stored in peek_count
      char const *stat_cursor = stat_buf;
      void *return_address = &&comm;
      next_word:;
      while (*stat_cursor++ != ' ') {
         if (stat_cursor == stat_buf_end) {
            // you're not supposed to do that
            napi_throw_error(env, "EMEM", "stat_cursor == stat_buf_end");
            return NULL;
         }
      }
      goto *return_address;

      // 02 comm
      {
         comm:
         //if (stat_cursor[0] == '(') {
            stat_cursor++;
         //} else {
            //goto die_statfd;
         //}

         size_t comm_len = 0;
         while (stat_cursor[comm_len] != ')') comm_len++;

         if (napi_create_string_utf8(env, stat_cursor, comm_len, &val) != napi_ok) {
            goto die_statfd;
         }

         if (napi_set_named_property(env, obj, "name", val) != napi_ok) {
            goto die_statfd;
         }

         // (init) S
         //  ^   ^
         //  ^   stat_cursor + comm_len
         //  stat_cursor

         // first go to closing paren, then jump twice to land on status
         stat_cursor += comm_len + LEN(") ");
      }

      // 03 state
      {
         if (napi_create_string_utf8(env, stat_cursor, 1, &val) != napi_ok) {
            goto die_statfd;
         }

         if (napi_set_named_property(env, obj, "state", val) != napi_ok) {
            goto die_statfd;
         }

         // (init) S 1
         //        ^
         //        stat_cursor

         // increment twice to land on ppid
         stat_cursor += LEN("S ");
      }


      // 04 ppid
      {
         uint32_t ppid = 0;
         while (*stat_cursor != ' ') {
            // assumed: '0' <= *stat_cursor <= '9'
            ppid *= 10;
            ppid += *stat_cursor - '0';
            stat_cursor++;
         }
         // cursor is currently on space, increment once to land on pgrp
         stat_cursor += LEN(" ");

         if (napi_create_uint32(env, ppid, &val) != napi_ok) {
            goto die_statfd;
         }

         if (napi_set_named_property(env, obj, "ppid", val) != napi_ok) {
            goto die_statfd;
         }
      }

      // 05 pgrp
      // 06 session
      // 07 tty_nr
      // 08 tpgid
      // 09 flags
      // 10 minflt
      // 11 cminflt
      // 12 majflt
      // 13 cmajflt
      // 14 utime
      // 15 stime
      // 16 cutime
      // 17 cstime
      {
         size_t words_to_skip = 13;
         pgrp_to_cstime:
         return_address = &&pgrp_to_cstime;
         // FIXME: extra compare for no reason
         while (words_to_skip --> 0) goto next_word;
      }

      // 18 priority
      {
         int32_t priority = 0;
         while (*stat_cursor != ' ') {
            // assumed: '0' <= *stat_cursor <= '9'
            priority *= 10;
            priority += *stat_cursor - '0';
            stat_cursor++;
         }

         // cursor is currently on space, increment once to get to nice
         stat_cursor += LEN(" ");

         if (napi_create_int32(env, priority, &val) != napi_ok) {
            goto die_statfd;
         }

         if (napi_set_named_property(env, obj, "priority", val) != napi_ok) {
            goto die_statfd;
         }
      }

      // 19 nice
      return_address = &&num_threads;
      goto next_word;

      // 20 num_threads
      num_threads: {
         int32_t threads = 0;
         while (*stat_cursor != ' ') {
            // assumed: '0' <= *stat_cursor <= '9'
            threads *= 10;
            threads += *stat_cursor - '0';
            stat_cursor++;
         }

         if (napi_create_int32(env, threads, &val) != napi_ok) {
            goto die_statfd;
         }

         if (napi_set_named_property(env, obj, "threads", val) != napi_ok) {
            goto die_statfd;
         }
      }

      ssize_t len = readlinkat(cur_dirfd, "exe", path_buf, PATH_MAX);
      if (len == -1) {
         goto die_statfd;
      }
      path_buf[len] = '\0';

      if (napi_create_string_utf8(env, path_buf, len, &val) != napi_ok) {
         goto die_statfd;
      }

      if (napi_set_named_property(env, obj, "path", val) != napi_ok) {
         goto die_statfd;
      }

      napi_set_element(env, ary, ary_idx++, obj);

      die_statfd:
      close(statfd);

      die_cur_dir:
      closedir(cur_dir);

      next_dirent:;
   }

   return ary;
}
