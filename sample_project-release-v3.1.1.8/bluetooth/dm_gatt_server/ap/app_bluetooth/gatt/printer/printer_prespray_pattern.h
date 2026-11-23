#ifndef PRINTER_PRESPRAY_PATTERN_H
#define PRINTER_PRESPRAY_PATTERN_H
#include "printer_nozzle_check.h"
#define PRINTER_PRESPRAY_ROW_ROUNDS 40U
#define PRINTER_PRESPRAY_PEAK_BITS 24U
#define PRINTER_PRESPRAY_BURST_ROUNDS 8U
/* Stationary: up to24 requests/column. A group receives at most8
 * consecutive-column shots (673us apart), then the next group runs.
 * Dose and row length remain unchanged; this is not a preheat waveform.
 */
static int printer_prespray_build(uint8_t *row,uint32_t bytes,unsigned rounds,const printer_map_config_t *cfg){
    if(!row || bytes<PRINTER_NC_BYTES || !rounds || rounds>PRINTER_PRESPRAY_ROW_ROUNDS || printer_nc_validate(cfg)) return -1;
    memset(row,0,PRINTER_NC_BYTES);
    for(unsigned ch=0;ch<3U;++ch)for(unsigned n=0;n<200U;++n){
        printer_map_route_t route;
        if(printer_nc_route(ch,n,cfg,&route)) return -1;
        unsigned index=ch*200U+n;
        for(unsigned batch=0;batch<rounds;++batch){
            unsigned base=(batch/PRINTER_PRESPRAY_BURST_ROUNDS)*PRINTER_PRESPRAY_BURST_ROUNDS;
            unsigned burst=rounds-base;
            if(burst>PRINTER_PRESPRAY_BURST_ROUNDS)burst=PRINTER_PRESPRAY_BURST_ROUNDS;
            unsigned col=40U+base*25U+(index/PRINTER_PRESPRAY_PEAK_BITS)*burst+(batch-base);
            row[col*100U+route.slot]|=(uint8_t)route.mask;
        }
    }
    return 0;
}
#endif
