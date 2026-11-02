#ifndef __BLE_PRINTER_SERVICE_H__
#define __BLE_PRINTER_SERVICE_H__



int printer_server_init(void);
int printer_ntf_data(uint8_t *buffer, uint16_t len);


#endif

