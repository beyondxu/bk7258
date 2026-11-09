/* Private experiment: render once, reuse the same column buffer twice.
 * No column SPI on blank return/feed; HC32 leaves CE low after each column.
 * Buffer CRC is a BK memory check, not an HC32 receive or physical-ink check.
 */
static int printer_execute_same_power_repeat(const uint8_t *columns) {
    if (!columns) return -4;
    const uint32_t bytes = PRINTER_PAGE_COLUMNS * 100U;
    const uint32_t expected = printer_crc32(columns, bytes);
    int result = 0;
    unsigned completed = 0;
    if (tph_mcu_set_head_power(0) != BK_OK) return -8;
    carriage_verified_power_up();
    if (tph_mcu_set_head_power(1) != BK_OK) { result = -8; goto stop; }
    os_printf("[REPEAT_POWER_BEGIN] copies=2 bytes=%lu crc32=%08lX settle_ms=%u one_head_ON no_rerender\r\n",
              (unsigned long)bytes, (unsigned long)expected, (unsigned)PRINTER_PAGE_HEAD_SETTLE_MS);
    rtos_delay_milliseconds(PRINTER_PAGE_HEAD_SETTLE_MS);
    for (unsigned copy = 1; copy <= 2; ++copy) {
        uint32_t before = printer_crc32(columns, bytes);
        if (before != expected) { result = -11; goto stop; }
        s_blank_spi_columns = 0;
        s_blank_spi_steps[0] = s_blank_spi_steps[1] = 0;
        s_blank_spi_failed = false;
        os_printf("[REPEAT_COPY_BEGIN] copy=%u/2 bytes=%lu crc32=%08lX head=ON\r\n",
                  copy, (unsigned long)bytes, (unsigned long)before);
        if (!carriage_stream_move_ramped(1, SHUTTLE_TOTAL_STEPS, columns)) { result = -9; goto stop; }
        rtos_delay_milliseconds(200);
        if (!carriage_stream_move_ramped(0, SHUTTLE_TOTAL_STEPS, NULL)) { result = -9; goto stop; }
        rtos_delay_milliseconds(300);
        uint32_t after = printer_crc32(columns, bytes);
        os_printf("[REPEAT_COPY_END] copy=%u/2 crc32=%08lX columns=%lu/1180 forward=%lu reverse=%lu head=ON ink_unverified\r\n",
                  copy, (unsigned long)after, (unsigned long)s_blank_spi_columns,
                  (unsigned long)s_blank_spi_steps[1], (unsigned long)s_blank_spi_steps[0]);
        if (after != expected) { result = -11; goto stop; }
        if (s_blank_spi_columns != PRINTER_PAGE_COLUMNS ||
            s_blank_spi_steps[0] != SHUTTLE_TOTAL_STEPS ||
            s_blank_spi_steps[1] != SHUTTLE_TOTAL_STEPS) { result = -9; goto stop; }
        ++completed;
        if (copy == 1) {
            uint32_t sent = 0;
            result = motor_page_feed_keep_power(PRINTER_PAGE_FEED_STEPS, PRINTER_PAGE_FEED_DIRECTION, &sent);
            os_printf("[REPEAT_FEED] pulses=%lu/%u result=%d head=ON boost_not_toggled distance_unverified\r\n",
                      (unsigned long)sent, (unsigned)PRINTER_PAGE_FEED_STEPS, result);
            if (result != BK_OK || sent != PRINTER_PAGE_FEED_STEPS) { result = -10; goto stop; }
            carriage_wake_on_powered_rail(); /* feed parked U4, not the head */
        }
    }
stop:
    /* Always request head OFF before removing the common rail. */
    if (tph_mcu_set_head_power(0) != BK_OK && !result) result = -8;
    bk_motor_set_state(0, 0);
    carriage_verified_power_down();
    os_printf("[REPEAT_POWER_END] result=%d completed=%u/2 head_OFF_requested no_blind_motion_on_failure\r\n", result, completed);
    return result;
}
