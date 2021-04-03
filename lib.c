#include "lib.h"
#include "js_native_api_types.h"
#include <psapi.h>
#include <tlhelp32.h>
#include <string.h>

napi_value w32_list_all(napi_env const restrict env, napi_callback_info const restrict info) {
   unused(info);

   napi_value ary;
   if (napi_create_array(env, &ary) != napi_ok) {
      napi_throw_error(env, "ENOCREATE", "Could not make output array!");
      return NULL;
   }

   HANDLE const snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

   if (snap == INVALID_HANDLE_VALUE) {
      napi_throw_error(env, "ENOTH32SNAP", "Snapshot handle was invalid!");
      return NULL;
   }

   PROCESSENTRY32W process = {.dwSize = sizeof(PROCESSENTRY32W)};

   if (Process32FirstW(snap, &process) == false) {
      CloseHandle(snap);
      return ary;
   }

   DWORD i = 0;
   do {
      napi_value obj;
      napi_value
         name,
         id,
         threads,
         parent_id,
         priority;

      if (napi_create_object(env, &obj) != napi_ok) {
         napi_throw_error(env, "ENOCREATE", "Could not make object!");
         return NULL;
      }

      if (napi_create_string_utf16(env, process.szExeFile, NAPI_AUTO_LENGTH, &name) != napi_ok) {
         napi_throw_error(env, "ENOCREATE", "Could not make string 'name'!");
         return NULL;
      }

      if (napi_set_named_property(env, obj, "name", name)) {
         napi_throw_error(env, "ENOSET", "Could not set property 'name' of object!");
         return NULL;
      }

      if (napi_create_uint32(env, process.th32ProcessID, &id) != napi_ok) {
         napi_throw_error(env, "ENOCREATE", "Could not make uint32 'id'!");
         return NULL;
      }

      if (napi_set_named_property(env, obj, "id", id)) {
         napi_throw_error(env, "ENOSET", "Could not set property 'id' of object!");
         return NULL;
      }

      if (napi_create_uint32(env, process.th32ProcessID, &threads) != napi_ok) {
         napi_throw_error(env, "ENOCREATE", "Could not make uint32 'threads'!");
         return NULL;
      }

      if (napi_set_named_property(env, obj, "threads", threads)) {
         napi_throw_error(env, "ENOSET", "Could not set property 'threads' of object!");
         return NULL;
      }

      if (napi_create_uint32(env, process.th32ParentProcessID, &parent_id) != napi_ok) {
         napi_throw_error(env, "ENOCREATE", "Could not make uint32 'parent_id'!");
         return NULL;
      }

      if (napi_set_named_property(env, obj, "parent_id", parent_id)) {
         napi_throw_error(env, "ENOSET", "Could not set property 'parent_id' of object!");
         return NULL;
      }

      if (napi_create_int32(env, process.pcPriClassBase, &priority) != napi_ok) {
         napi_throw_error(env, "ENOCREATE", "Could not make int32 priority!");
         return NULL;
      }

      if (napi_set_named_property(env, obj, "priority", priority)) {
         napi_throw_error(env, "ENOSET", "Could not set property 'priority' of object!");
         return NULL;
      }

      HANDLE const process_handle = OpenProcess(PROCESS_QUERY_INFORMATION, 0, process.th32ProcessID);

      if (process_handle == NULL) {
         // napi_throw_error(env, "ENOPROCESSHANDLE", "Could not open process handle!");
         // return NULL;
         goto push;
      }

      WCHAR path_ary[MAX_PATH];
      if (GetModuleFileNameExW(process_handle, NULL, path_ary, MAX_PATH) == false) {
         // napi_throw_error(env, "ENOMODULEFILENAME", "Could not get module file name!");
         // return NULL;
         goto push;
      }

      napi_value path;
      if (napi_create_string_utf16(env, path_ary, NAPI_AUTO_LENGTH, &path) != napi_ok) {
         napi_throw_error(env, "ENOCREATE", "Could not make string 'path'!");
         return NULL;
      }

      if (napi_set_named_property(env, obj, "path", path) != napi_ok) {
         napi_throw_error(env, "ENOSET", "Could not set property 'path' of object!");
         return NULL;
      }

      push: napi_set_element(env, ary, i++, obj);
   } while (Process32NextW(snap, &process));

   CloseHandle(snap);
   return ary;
}

napi_value init_all(napi_env const env, napi_value exports) {

   if (napi_create_function(env, "w32_list_all", NAPI_AUTO_LENGTH, w32_list_all, NULL, &exports) != napi_ok) {
      napi_throw_error(env, "ENOCREATE", "Could not create function!");
      return NULL;
   }

   return exports;
}

NAPI_MODULE(native_node_module, init_all)
