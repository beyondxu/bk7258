#ifndef PRINTER_MAINTENANCE_H
#define PRINTER_MAINTENANCE_H
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
/*06/08: empty params = waste-pad clean; param00 = paper check,
 * param01 = paper recovery, param02 = waste-pad clean (no paper feed). */
enum {PRINTER_MAINT_CHECK=0,PRINTER_MAINT_CLEAN=1,PRINTER_MAINT_WASTE=2};
enum {PRINTER_MAINT_ACCEPTED=0,PRINTER_MAINT_BUSY=2,PRINTER_MAINT_INVALID=3,
      PRINTER_MAINT_UNAVAILABLE=4,PRINTER_MAINT_PRECONDITION=5};
bool hw_printer_start_maintenance_async(uint8_t mode);
void printer_protocol_maintenance_complete(uint8_t mode,int result);
#ifdef __cplusplus
}
#endif
#endif
