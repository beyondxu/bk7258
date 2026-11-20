#ifndef PRINTER_MAINTENANCE_PATTERN_H
#define PRINTER_MAINTENANCE_PATTERN_H
#include "printer_nozzle_check.h"
#include "printer_maintenance.h"
/* Ordinary-drive requests per nozzle:16 before,32 recovery,16 after.
 * All output fits the existing1180-column paper window; peak<=8 bits/column. */
#define PRINTER_MAINT_PHASES 3U
#define PRINTER_MAINT_PEAK_BITS 8U
static unsigned printer_maint_dots(unsigned phase){return phase==1U?32U:16U;}
static int printer_maint_build(uint8_t *out,uint32_t bytes,unsigned mode,
                              unsigned color,unsigned group,unsigned phase,
                              const printer_map_config_t *cfg){
 if(mode>PRINTER_MAINT_CLEAN||phase>=3U||color>=3U||group>=4U||!out||bytes<PRINTER_NC_BYTES)return -1;
 if(mode==PRINTER_MAINT_CHECK)return printer_nc_build(out,bytes,color,group,cfg);
 const uint32_t start[3]={40U,420U,800U};
 unsigned dots=printer_maint_dots(phase);
 memset(out,0,PRINTER_NC_BYTES);
 for(unsigned n=0;n<50U;++n){
  printer_map_route_t r;
  if(printer_nc_route(color,group*50U+n,cfg,&r)||r.slot>=100U||r.slot%4U!=group||!r.mask||r.mask>32U)return -1;
  uint32_t first=start[phase]+n*5U+r.delay_columns;
  if(first+dots>1180U)return -1;
  for(uint32_t x=first;x<first+dots;++x)out[x*100U+r.slot]|=(uint8_t)r.mask;
 }
 for(unsigned x=0;x<1180U;++x){
  unsigned bits=0;
  for(unsigned slot=0;slot<100U;++slot){unsigned v=out[x*100U+slot];for(unsigned b=0;b<6U;++b)bits+=(v>>b)&1U;}
  if(bits>PRINTER_MAINT_PEAK_BITS)return -1;
 }
 return 0;
}
static void printer_maint_reverse(uint8_t *row){
 for(unsigned left=0;left<590U;++left){unsigned right=1179U-left;for(unsigned slot=0;slot<100U;++slot){uint8_t v=row[left*100U+slot];row[left*100U+slot]=row[right*100U+slot];row[right*100U+slot]=v;}}
}
#endif
