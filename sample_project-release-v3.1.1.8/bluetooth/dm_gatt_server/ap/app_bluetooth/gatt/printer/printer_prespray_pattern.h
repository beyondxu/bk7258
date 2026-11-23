#ifndef PRINTER_PRESPRAY_PATTERN_H
#define PRINTER_PRESPRAY_PATTERN_H
#include "printer_nozzle_check.h"
#define PRINTER_PRESPRAY_ROW_ROUNDS 40U
#define PRINTER_PRESPRAY_PEAK_BITS 24U
/* Stationary: no image-X compensation.24 requests per column,25 columns
 * per600-nozzle round. Each nozzle repeats after25 columns (~16.8ms).
 * Up to40 rounds fit one finite1180-column transport row.
 */
static int printer_prespray_build(uint8_t *row,uint32_t bytes,unsigned rounds,const printer_map_config_t *cfg){
    if(!row || bytes<PRINTER_NC_BYTES || !rounds || rounds>PRINTER_PRESPRAY_ROW_ROUNDS || printer_nc_validate(cfg)) return -1;
    memset(row,0,PRINTER_NC_BYTES);
    for(unsigned ch=0;ch<3U;++ch)for(unsigned n=0;n<200U;++n){
        printer_map_route_t route;
        if(printer_nc_route(ch,n,cfg,&route)) return -1;
        unsigned index=ch*200U+n;
        for(unsigned batch=0;batch<rounds;++batch){
            unsigned col=40U+batch*25U+index/PRINTER_PRESPRAY_PEAK_BITS;
            row[col*100U+route.slot]|=(uint8_t)route.mask;
        }
    }
    return 0;
}
#endif
