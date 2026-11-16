/* One dry-run relative to the ORIGINAL known scan start, never a homing routine. */
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
    const uint8_t direction[4]={0,1,0,1};
    const uint32_t steps[4]={PRINTER_SERVICE_MARGIN_STEPS,PRINTER_SERVICE_SPAN_STEPS,
                            PRINTER_SERVICE_SPAN_STEPS,PRINTER_SERVICE_MARGIN_STEPS};
    int result=0;
    int32_t relative=0;
    motor_carriage_test_power_up();
    os_printf("[SERVICE_BEGIN] old_origin=0 left=-1870 right=9350 span=11220 head_OFF all_regions_non_firing feed=0 copies=1 no_homing\r\n");
    for (unsigned leg=0;leg<4U;++leg) {
        uint32_t sent=0;
        int code=motor_carriage_service_leg(direction[leg],steps[leg],&sent);
        relative += direction[leg] ? (int32_t)sent : -(int32_t)sent;
        os_printf("[SERVICE_POSITION] leg=%u/4 relative_pulses=%ld result=%d measured_position_unverified\r\n",
                  leg+1U,(long)relative,code);
        if (code!=BK_OK || sent!=steps[leg]) {result=-9;break;}
        rtos_delay_milliseconds(300);
    }
    bk_motor_set_state(1,0);
    motor_all_power_down();
    os_printf("[SERVICE_END] code=%d relative_pulses=%ld motor_OFF boost_OFF no_automatic_repeat no_blind_return\r\n",
              result,(long)relative);
    return result;
}
#endif
