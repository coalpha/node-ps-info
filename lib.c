#include "lib.h"

#if defined _WIN64
#include "win32.c"
#define fn_name "win32_process_list"
#elif defined __APPLE__ || __FreeBSD__
#include "darwin.c"
#elif defined __linux__
#include "linux.c"
#else
#error "@coalpha/ps-list: Unrecognized platform!"
#endif

napi_value init_all(napi_env const env, napi_value exports) {
   if (napi_create_function(env, fn_name, NAPI_AUTO_LENGTH, process_list, NULL, &exports) != napi_ok) {
      napi_throw_error(env, "ENOCREATE", "Could not export process_list!");
      return NULL;
   }
   return exports;
}

NAPI_MODULE(process_list, init_all)
