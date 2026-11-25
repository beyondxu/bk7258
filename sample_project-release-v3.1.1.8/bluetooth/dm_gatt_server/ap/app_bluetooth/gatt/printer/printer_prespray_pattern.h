#ifndef PRINTER_PRESPRAY_PATTERN_H
#define PRINTER_PRESPRAY_PATTERN_H
#include "printer_nozzle_check.h"
#define PRINTER_PRESPRAY_ROW_ROUNDS 40U
#include "printer_line_protocol.h"
#define PRINTER_PRESPRAY_PEAK_BITS (2U * PL_PRESPRAY_SHORT_ADDRESSES)
#define PRINTER_PRESPRAY_BURST_ROUNDS PL_PRESPRAY_BURST_MAX
#define PRINTER_PRESPRAY_GROUPS 24U
/* Each column uses one D and a pair of S sections (S1/S2 or S3/S4).
 * Split 25 addresses into 13+12 groups for the HC short scan. Each nozzle
 * stays at the same offset in the address train for all shots in its burst.
 */
static int printer_prespray_build(uint8_t *row,uint32_t bytes,unsigned rounds,const printer_map_config_t *cfg){
    if(!row || bytes<PRINTER_NC_BYTES || !rounds || rounds>PRINTER_PRESPRAY_ROW_ROUNDS || printer_nc_validate(cfg)) return -1;
    memset(row,0,PRINTER_NC_BYTES);
    for(unsigned ch=0;ch<3U;++ch)for(unsigned n=0;n<200U;++n){
        printer_map_route_t route;
        if(printer_nc_route(ch,n,cfg,&route)) return -1;
        unsigned domain=0U;
        while(domain<6U && route.mask!=(1U<<domain))++domain;
        if(domain>=6U)return -1;
        unsigned group=domain*4U+((route.slot%4U)/2U)*2U+(route.slot/4U)/PL_PRESPRAY_SHORT_ADDRESSES;
        for(unsigned batch=0;batch<rounds;++batch){
            unsigned base=(batch/PRINTER_PRESPRAY_BURST_ROUNDS)*PRINTER_PRESPRAY_BURST_ROUNDS;
            unsigned burst=rounds-base;
            if(burst>PRINTER_PRESPRAY_BURST_ROUNDS)burst=PRINTER_PRESPRAY_BURST_ROUNDS;
            unsigned col=40U+base*PRINTER_PRESPRAY_GROUPS+group*burst+(batch-base);
            row[col*100U+route.slot]|=(uint8_t)route.mask;
        }
    }
    return 0;
}
#endif
