#ifndef __BLE_PRINTER_SERVICE_H__
#define __BLE_PRINTER_SERVICE_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int printer_server_init(void);
int printer_ntf_data(uint8_t *buffer, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* __BLE_PRINTER_SERVICE_H__ */

