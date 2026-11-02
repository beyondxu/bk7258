#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include "media_service.h"
#include "kws_demo.h"

int main(void)
{
    bk_init();

    media_service_init();

    bk_kws_demo_init();

    return 0;
}
