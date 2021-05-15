#include "lib.h"

#if defined _WIN64
#include "win32.c"
#define fn_name "win32_process_list"
#elif defined __APPLE__ || __FreeBSD__
#include "darwin.c"
#elif defined __linux__
#include "linux.c"
#else
#error "@coalpha/ps_list: Unrecognized platform!"
#endif

NAPI_MODULE(ps_list, init_all)
