#include "adiak.h"
#include "adiak_internal.h"


static adiak_tool_t *local_tool_list = NULL;

adiak_t adiak_public = { ADIAK_VERSION, ADIAK_VERSION, 0, 1, &local_tool_list };

int adiak_launchdate()
{
    return -1;
}

int adiak_executable()
{
    return -1;
}

int adiak_executablepath()
{
    return -1;
}

int adiak_libraries()
{
    return -1;
}

int adiak_cmdline()
{
    return -1;
}

int adiak_measure_walltime()
{
    return 0;
}

adiak_t* adiak_sys_init()
{
    return &adiak_public;
}
