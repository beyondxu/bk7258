/* Isolated reverse-scan experiment; the verified original movement is intact.
 * Forward fires at physical STEP 200+6*k. On return position is 7480-i:
 * choose i=206+6*j, data k=1179-j, giving identical physical coordinates.
 */
static bool carriage_return_compare_move(uint8_t direction, uint32_t total_steps, const uint8_t *columns) {
    if (total_steps != 7480U || direction > 1U || (direction && columns)) return false;
    if (direction) GPIO_UP(TPH_MOT_DIR); else GPIO_DOWN(TPH_MOT_DIR);
    delay_us(50);
    const uint32_t start_delay_us = 3000U, cruise_delay_us = 260U;
    for (uint32_t i = 0; i < total_steps; ++i) {
        if (!direction && columns && i >= 206U && i <= 7280U && (i - 206U) % 6U == 0U) {
            if (s_blank_spi_columns >= 1180U ||
                tph_fire_raw_column_bytes(columns + (1179U - s_blank_spi_columns) * 100U) != BK_OK) {
                s_blank_spi_failed = true;
                return false;
            }
            ++s_blank_spi_columns;
        }
        uint32_t delay;
        if (i < 250U) delay = start_delay_us - (start_delay_us - cruise_delay_us) * i / 250U;
        else if (i >= total_steps - 250U) delay = start_delay_us - (start_delay_us - cruise_delay_us) * (total_steps - 1U - i) / 250U;
        else delay = cruise_delay_us;
        GPIO_UP(TPH_MOT_STEP);
        delay_us(4);
        GPIO_DOWN(TPH_MOT_STEP);
        delay_us(delay);
        ++s_blank_spi_steps[direction ? 1 : 0];
    }
    return true;
}
static int printer_execute_section_return_line(const uint8_t *columns) {
    int result = 0;
    if (!columns) return -1;
    s_blank_spi_columns = 0;
    s_blank_spi_steps[0] = s_blank_spi_steps[1] = 0;
    s_blank_spi_failed = false;
    if (tph_mcu_set_head_power(0) != BK_OK) return -8;
    carriage_verified_power_up();
    os_printf("[RETURN_LINE_BEGIN] forward=blank_head_OFF reverse=ink source_columns=1179..0 reverse_fire_steps=206..7280 total_steps_each=7480 feed=0\r\n");
    if (!carriage_return_compare_move(1, 7480U, NULL)) { result = -9; goto stop; }
    rtos_delay_milliseconds(200);
    if (tph_mcu_set_head_power(1) != BK_OK) { result = -8; goto stop; }
    if (!carriage_return_compare_move(0, 7480U, columns)) { result = -9; goto stop; }
    if (tph_mcu_set_head_power(0) != BK_OK) { result = -8; goto stop; }
    rtos_delay_milliseconds(300);
stop:
    if (result) {
        (void)tph_mcu_set_head_power(0);
        os_printf("[RETURN_LINE_STOP] code=%d no_blind_reposition\r\n", result);
    }
    carriage_verified_power_down();
    os_printf("[RETURN_LINE_END] result=%d columns=%lu/1180 forward=%lu reverse=%lu ink_and_position_unverified\r\n",
              result, (unsigned long)s_blank_spi_columns,
              (unsigned long)s_blank_spi_steps[1], (unsigned long)s_blank_spi_steps[0]);
    return result;
}
