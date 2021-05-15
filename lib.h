#define NAPI_VERSION 3
#include <node_api.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

napi_value init_all(napi_env const env, napi_value exports);

napi_value ps_list(
   napi_env const restrict env,
   napi_callback_info const restrict info
);
