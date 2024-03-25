#include "ps_list.h"

#if defined _WIN64
#include "win32.c"
#elif defined __APPLE__ || __FreeBSD__
#include "darwin.c"
#elif defined __linux__
#include "linux.c"
#else
#error "@falfiya/ps-list: Unrecognized platform!"
#endif

NAPI_MODULE(ps_list, init_all)
