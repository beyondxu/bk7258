/* Start manually at LEFT service position, move RIGHT once. No auto-homing. */
#ifndef PRINTER_SERVICE_TRAVEL_TEST_H
#define PRINTER_SERVICE_TRAVEL_TEST_H
#include "printer_service_geometry.h"
static int printer_run_service_travel_test(void) {
    static bool attempted;
    if (attempted || !printer_link_is_ready()) {
        os_printf("[SERVICE_BLOCKED] already_attempted_or_link_unavailable no_motion\r\n");
        return -7;
    }
    attempted=true;
    if (tph_mcu_set_head_power(0)!=BK_OK) {
        os_printf("[SERVICE_END] code=-8 head_OFF_not_acknowledged no_motion\r\n");
        return -8;
    }
    uint32_t sent=0;
    motor_carriage_test_power_up();
    os_printf("[SERVICE_BEGIN] origin=LEFT_SERVICE zero=0 right=%lu legacy_start=%lu legacy_end=%lu head_OFF right_only=1 feed=0 no_homing\r\n",
              (unsigned long)PRINTER_SERVICE_SPAN_STEPS,
              (unsigned long)PRINTER_SERVICE_MARGIN_STEPS,
              (unsigned long)(PRINTER_SERVICE_MARGIN_STEPS+PRINTER_SERVICE_BASE_STEPS));
    int code=motor_carriage_service_leg(1U,PRINTER_SERVICE_SPAN_STEPS,&sent);
    int result=code==BK_OK && sent==PRINTER_SERVICE_SPAN_STEPS ? 0 : -9;
    os_printf("[SERVICE_POSITION] leg=1/1 relative_pulses=%lu result=%d measured_position_unverified\r\n",
              (unsigned long)sent,code);
    bk_motor_set_state(1,0);
    motor_all_power_down();
    os_printf("[SERVICE_END] code=%d relative_pulses=%lu motor_OFF boost_OFF stopped_at_right no_automatic_repeat no_return\r\n",
              result,(unsigned long)sent);
    return result;
}
#endif
