#define NAPI_VERSION 3
#include <node_api.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

napi_value process_list(
   napi_env const restrict env,
   napi_callback_info const restrict info
);
