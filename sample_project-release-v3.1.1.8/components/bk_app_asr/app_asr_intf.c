#include "app_asr_intf.h"

const app_asr_intf_t *app_asr_intf_instance(void)
{
#if CONFIG_BEKEN_KWS
    extern const app_asr_intf_t *kws_asr_instance(void);
    return kws_asr_instance();
#else
    return NULL;
#endif
}