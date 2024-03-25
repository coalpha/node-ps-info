#include "js_native_api.h"
#include "ps_list.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

#ifdef __clang__
#pragma clang diagnostic ignored "-Wtautological-compare"
#endif

napi_value process_list(napi_env const env, napi_callback_info const info) {
   bool with_paths = false;
   {
      napi_value argv[1];
      size_t argc = sizeof(argv);
      napi_value this;
      void *data;


      napi_ok
      == napi_ok && napi_get_cb_info(env, info, &argc, argv, &this, &data)
      == napi_ok && napi_coerce_to_bool(env, argv[0], argv)
      == napi_ok && napi_get_value_bool(env, argv[0], &with_paths);
   }

   napi_value ary;
   
   if (napi_create_array(env, &ary) != napi_ok) {
      napi_throw_error(env, "NOCREATE", "Could not make output array!");
      return NULL;
   }

   HANDLE const snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS | TH32CS_SNAPMODULE, 0);

   if (snap == INVALID_HANDLE_VALUE) {
      napi_throw_error(env, "NOTH32SNAP", "Snapshot handle was invalid!");
      return NULL;
   }

   PROCESSENTRY32W process = {.dwSize = sizeof(PROCESSENTRY32W)};

   if (Process32FirstW(snap, &process) == false) {
      goto cleanup;
   }

   DWORD i = 0;
   napi_value obj;
   napi_value name;
   napi_value id;
   napi_value threads;
   napi_value parent_id;
   napi_value priority;
   do {
      napi_ok
      == napi_ok && napi_create_object(env, &obj)

      == napi_ok && napi_create_string_utf16(env, process.szExeFile, NAPI_AUTO_LENGTH, &name)
      == napi_ok && napi_create_uint32(env, process.th32ProcessID, &id)
      == napi_ok && napi_create_uint32(env, process.th32ProcessID, &threads)
      == napi_ok && napi_create_uint32(env, process.th32ParentProcessID, &parent_id)
      == napi_ok && napi_create_int32 (env, process.pcPriClassBase, &priority)

      == napi_ok && napi_set_named_property(env, obj, "name", name)
      == napi_ok && napi_set_named_property(env, obj, "id", id)
      == napi_ok && napi_set_named_property(env, obj, "threads", threads)
      == napi_ok && napi_set_named_property(env, obj, "parent_id", parent_id)
      == napi_ok && napi_set_named_property(env, obj, "priority", priority);

      if (with_paths) {
         HANDLE const process_handle = OpenProcess(PROCESS_QUERY_INFORMATION, 0, process.th32ProcessID);
         WCHAR path_ary[MAX_PATH];
         napi_value path;

         true
         && process_handle != NULL
         && GetModuleFileNameExW(process_handle, NULL, path_ary, MAX_PATH)
         && napi_create_string_utf16(env, path_ary, NAPI_AUTO_LENGTH, &path) == napi_ok
         && napi_set_named_property(env, obj, "path", path);
      }

      napi_set_element(env, ary, i++, obj);
   } while (Process32NextW(snap, &process));

   cleanup:
   CloseHandle(snap);
   return ary;
}
