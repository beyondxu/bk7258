#include "os/os.h"

int moduleA(void)
{
    bk_printf("[%s][%d]new\r\n", __FUNCTION__, __LINE__);
    return 0;
}
