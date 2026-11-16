/* Each image scan starts/ends at the verified LEFT service position.
 * Only the legacy 7480-step image segment may enable the head or send columns.
 */
#ifndef PRINTER_SERVICE_PRINT_H
#define PRINTER_SERVICE_PRINT_H
#include "printer_service_geometry.h"
static int printer_service_move(const char *stage, uint8_t direction, uint32_t steps) {
    uint32_t sent=0;
    int result=motor_carriage_service_leg(direction,steps,&sent);
    s_blank_spi_steps[direction?1:0]+=sent;
    os_printf("[PRINT_SERVICE_MOVE] stage=%s dir=%u pulses=%lu/%lu head_OFF no_column_SPI physical_position_unverified\r\n",
              stage,direction,(unsigned long)sent,(unsigned long)steps);
    return result==BK_OK && sent==steps ? 0 : -9;
}
static int printer_execute_service_line(const uint8_t *columns) {
    int result=0;
    s_blank_spi_columns=0;
    s_blank_spi_steps[0]=s_blank_spi_steps[1]=0;
    s_blank_spi_failed=false;
    if (!columns) return -1;
    if (tph_mcu_set_head_power(0)!=BK_OK) return -8;
    carriage_verified_power_up();
    result=printer_service_move("LEAVE_LEFT",1U,PRINTER_SERVICE_MARGIN_STEPS);
    if (result) goto stop;
    if (tph_mcu_set_head_power(1)!=BK_OK) {result=-8;goto stop;}
#if PRINTER_PAGE_HEAD_SETTLE_MS > 0
    os_printf("[PAGE_HEAD_SETTLE] delay_ms=%u head=ON at_legacy_print_start no_column_SPI\r\n",(unsigned)PRINTER_PAGE_HEAD_SETTLE_MS);
    rtos_delay_milliseconds(PRINTER_PAGE_HEAD_SETTLE_MS);
#endif
    os_printf("[PRINT_WINDOW_BEGIN] service_origin=LEFT legacy_steps=7480 columns=1180 steps_per_column=6\r\n");
    if (!carriage_stream_move_ramped(1U,PRINTER_SERVICE_BASE_STEPS,columns)) {result=-9;goto stop;}
    if (s_blank_spi_columns!=1180U) {result=-9;goto stop;}
    if (tph_mcu_set_head_power(0)!=BK_OK) {result=-8;goto stop;}
    result=printer_service_move("TO_RIGHT",1U,PRINTER_SERVICE_MARGIN_STEPS);
    if (result) goto stop;
    rtos_delay_milliseconds(200);
    result=printer_service_move("RETURN_LEFT",0U,PRINTER_SERVICE_SPAN_STEPS);
    if (result) goto stop;
    rtos_delay_milliseconds(300);
stop:
    if (result) {
        (void)tph_mcu_set_head_power(0);
        os_printf("[PRINT_SERVICE_STOP] code=%d no_blind_return restore_origin_before_retry\r\n",result);
    }
    carriage_verified_power_down();
    os_printf("[PRINT_SERVICE_END] code=%d columns=%lu/1180 forward=%lu reverse=%lu relative_pulses=%ld head_OFF boost_OFF ink_and_position_unverified\r\n",
              result,(unsigned long)s_blank_spi_columns,(unsigned long)s_blank_spi_steps[1],
              (unsigned long)s_blank_spi_steps[0],(long)((int32_t)s_blank_spi_steps[1]-(int32_t)s_blank_spi_steps[0]));
    return result;
}
#endif
