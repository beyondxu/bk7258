#ifndef PRINTER_INK_IDLE_H
#define PRINTER_INK_IDLE_H
/* Worker/boot context only. Timestamp records completed requests, not observed drops. */
static uint64_t s_prespray_last_ink_us;
static unsigned s_prespray_have_ink;
static void printer_prespray_note_ink(void){s_prespray_last_ink_us=bk_aon_rtc_get_us();s_prespray_have_ink=1U;}
static unsigned printer_prespray_plan(uint64_t now,uint64_t *idle_us,unsigned *known){
    *known=s_prespray_have_ink && now>=s_prespray_last_ink_us;
    *idle_us=*known?now-s_prespray_last_ink_us:0U;
    if(!*known || *idle_us>=UINT64_C(900000000))return 400U;
    if(*idle_us>=UINT64_C(180000000))return 100U;
    return 10U;
}
#endif
