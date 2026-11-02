#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include "apconfig_example.h"

int main(void)
{
    bk_init();

    wifi_provisioning_web_config_start();

    return 0;
}
