#include "lib.h"

#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h> // PATH_MAX

// max PID could be 4194304.
// "/proc/".length + "4194304".length + "/stat".length + 1
//               6 +                7 +             4 +
static char proc_path[18] = "/proc/";
static char *restrict proc_path6 = proc_path + 6;

napi_value ps_list(napi_env const restrict env, napi_callback_info const restrict info) {
   (void) info;

   napi_value ary;
   if (napi_create_array(env, &ary) != napi_ok) {
      napi_throw_error(env, "ENOCREATE", "Could not make output array!");
      return NULL;
   }

   DIR *const restrict proc = opendir(proc_path);
   if (!proc) {
      return ary;
   }

   size_t ary_idx = 0;
   napi_value obj;

   napi_value id;
   napi_value path;
   struct dirent *restrict dirent;
   while ((dirent = readdir(proc))) {
      // simultaniously check if the directory is a PID, copy it into the
      // proc_path, and parse it's name as a u32.
      char c;
      uint32_t pid = 0;
      size_t i = 0;
      while ((c = dirent->d_name[i])) {
         if (c < '0' ||'9' < c) {
            goto skip_current_dirent;
         }

         pid *= 10;
         pid += c - '0';
         proc_path6[i] = c;
         i++;
      }

      if (napi_create_object(env, &obj) != napi_ok) {
         return ary;
      }

      if (napi_create_uint32(env, pid, &id) != napi_ok) {
         return ary;
      }

      if (napi_set_named_property(env, obj, "id", id) != napi_ok) {
         return ary;
      }

      proc_path6[i + 0] = '/';
      proc_path6[i + 1] = 'e';
      proc_path6[i + 2] = 'x';
      proc_path6[i + 3] = 'e';
      char exe_path[PATH_MAX];

      ssize_t len = readlink(proc_path, exe_path, PATH_MAX);
      if (len == -1) {
         goto skip_current_dirent;
      }

      exe_path[len] = '\0';
      if (napi_create_string_utf8(env, exe_path, len, &path) == napi_ok) {
         napi_set_named_property(env, obj, "path", path);
      }

      napi_set_element(env, ary, ary_idx++, obj);
      skip_current_dirent:;
   };

   return ary;
}
