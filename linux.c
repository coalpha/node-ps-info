#include "lib.h"

#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h> // PATH_MAX
#include <fcntl.h>  // openat
#include <stdio.h>

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

   napi_value id;
   napi_value name;
   napi_value path;
   struct dirent *restrict dirent;
   while ((dirent = readdir(proc))) {
      if (napi_create_object(env, &obj) != napi_ok) {
         return ary;
      }

      // simultaniously check if the directory is a PID, copy it into the
      // proc_path, and parse it's name as a u32.
      uint32_t pid = 0;
      {
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
      }

      if (napi_create_uint32(env, pid, &id) != napi_ok) {
         return ary;
      }

      if (napi_set_named_property(env, obj, "id", id) != napi_ok) {
         return ary;
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
         + LEN(S_U32_MAX)  // 16 cutime
         + LEN(" ")
         + LEN(S_U32_MAX)  // 17 cstime
         + LEN(" ")
         + LEN(S_U32_MAX)  // 18 priority
         + LEN(" ")
         + LEN(S_U32_MAX)  // 19 nice
         + LEN(" ")
         + LEN(S_U32_MAX)  // 20 num_threads
         + LEN("\0")
      ];

      char const *const stat_buf_end = stat_buf + sizeof(stat_buf);

      if (read(statfd, stat_buf, sizeof(stat_buf)) == -1) {
         goto die_statfd;
      }

      // manually inlined function
      // to invoke, set delim and return_address
      // result is stored in peek_count
      char const *stat_cursor = stat_buf;
      char delim = ' ';
      void *return_address = &&comm;
      peek:
      (void) 0;
      size_t peek_count = 0;
      while (stat_cursor[peek_count] != delim) {
         if (stat_cursor + peek_count == stat_buf_end) {
            // you're not supposed to do that
            napi_throw_error(env, "EMEM", "stat_cursor == stat_buf_end");
            return NULL;
         }
         peek_count++;
      }
      goto *return_address;

      comm:
      stat_cursor += peek_count + 1;
      // this should never be the case
      if (stat_cursor[0] == '(') {
         stat_cursor++;
      } else {
         goto die_statfd;
      }

      delim = ')';
      return_address = &&comm_end;
      goto peek;

      comm_end:
      if (napi_create_string_utf8(env, stat_cursor, peek_count, &name) != napi_ok) {
         goto die_statfd;
      }

      if (napi_set_named_property(env, obj, "name", name) != napi_ok) {
         goto die_statfd;
      }

      char exe_path[PATH_MAX];
      ssize_t len = readlinkat(cur_dirfd, "exe", exe_path, PATH_MAX);
      if (len == -1) {
         goto die_statfd;
      }
      exe_path[len] = '\0';

      if (napi_create_string_utf8(env, exe_path, len, &path) != napi_ok) {
         goto die_statfd;
      }

      if (napi_set_named_property(env, obj, "path", path) != napi_ok) {
         goto die_statfd;
      }

      napi_set_element(env, ary, ary_idx++, obj);

      die_statfd:
      close(statfd);

      die_cur_dir:
      closedir(cur_dir);

      next_dirent:
      (void) 0;
   };

   return ary;
}
