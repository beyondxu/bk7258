#ifndef PRINTER_FIXED_S2_BASELINE
#define PRINTER_FIXED_S2_BASELINE 0
#endif
#if PRINTER_FIXED_S2_BASELINE
#include "printer_fixed_s2.h"
#endif
#ifndef PRINTER_STARTUP_RECOVERY
#define PRINTER_STARTUP_RECOVERY 0
#endif
#if PRINTER_STARTUP_RECOVERY
/* Expected wire order is x0,x0,x1,x1,..., NOT two full-frame copies.
 * Compute only while idle before the head is powered; never hash inside a pulse.
 */
static uint32_t printer_recovery_crc_update(uint32_t crc, const uint8_t *columns, unsigned repeats) {
    for (unsigned x = 0; x < PRINTER_PAGE_COLUMNS; ++x) {
        for (unsigned r = 0; r < repeats; ++r)
            crc = printer_crc32_update(crc, columns + x * 100U, 100U);
        if ((x & 63U) == 63U) printer_map_yield_cpu();
    }
    return crc;
}
#endif
/* Private, APP-triggered OFF/restart vs powered-idle comparison.
 * Requires HC32 CONT_CE_v1: every column must STOP and RESET before returning.
 * No startup motion, no priming, no extra column SPI during return/feed/idle.
 */
#define PRINTER_IDLE_AB_BYTES (PRINTER_PAGE_COLUMNS * 100U)
#ifndef PRINTER_IDLE_HELD_ONLY
#define PRINTER_IDLE_HELD_ONLY 0 /* retained AB mode for older configurations */
#endif
#ifndef PRINTER_IDLE_OFF_ONLY
#define PRINTER_IDLE_OFF_ONLY 0
#endif
#ifndef PRINTER_IDLE_REFERENCE_ONLY
#define PRINTER_IDLE_REFERENCE_ONLY 0
#endif
#if (PRINTER_IDLE_OFF_ONLY + PRINTER_IDLE_HELD_ONLY + PRINTER_IDLE_REFERENCE_ONLY) > 1
#error Select only one single-phase idle mode
#endif
#define PRINTER_IDLE_SINGLE_PHASE (PRINTER_IDLE_HELD_ONLY || PRINTER_IDLE_OFF_ONLY || PRINTER_IDLE_REFERENCE_ONLY)
#ifndef PRINTER_IDLE_AB_GAP_MS
#define PRINTER_IDLE_AB_GAP_MS 60000U
#endif
#if PRINTER_IDLE_AB_GAP_MS < 10000U || PRINTER_IDLE_AB_GAP_MS > 600000U
#error Idle comparison supports 10..600 seconds
#endif
#define PRINTER_IDLE_AB_GAP_US ((uint64_t)PRINTER_IDLE_AB_GAP_MS * 1000U)
#define PRINTER_IDLE_AB_LINE_FEED 780U
#define PRINTER_IDLE_AB_GROUP_FEED 1560U

/* Sleep in bounded chunks, feeding the scheduler; no long busy delay. */
static void printer_idle_ab_wait_until(uint64_t deadline, const char *phase, const char *state) {
    unsigned last_bucket = 0;
    for (;;) {
        uint64_t now = bk_aon_rtc_get_us();
        if (now >= deadline) return;
        uint64_t remaining = deadline - now;
        unsigned bucket = (unsigned)(remaining / 10000000ULL) + 1U;
        if (bucket != last_bucket) {
            os_printf("[IDLE_AB_WAIT] phase=%s state=%s remaining_ms=%lu no_column_SPI\r\n",
                      phase, state, (unsigned long)(remaining / 1000U));
            last_bucket = bucket;
        }
        if (remaining >= 1000U) {
            uint32_t ms = (uint32_t)(remaining / 1000U);
            rtos_delay_milliseconds(ms > 1000U ? 1000U : ms);
        } else delay_us((uint32_t)remaining);
    }
}

static void printer_idle_ab_park_carriage(void) {
    GPIO_UP(TPH_MOT_EN);
    GPIO_DOWN(TPH_MOT_SLEEP);
    GPIO_DOWN(TPH_MOT_RES);
    /* Do not touch shared boost, GPIO configuration, or HC32 power. */
}

static int printer_idle_ab_scan(const uint8_t *columns, uint32_t crc,
                               const char *phase, const char *kind, unsigned copy,
                               uint64_t idle_anchor, uint64_t *lead_us) {
    if (printer_crc32(columns, PRINTER_IDLE_AB_BYTES) != crc) return -11;
    s_blank_spi_columns = 0;
    s_blank_spi_steps[0] = s_blank_spi_steps[1] = 0;
    s_blank_spi_failed = false;
#if PRINTER_STARTUP_RECOVERY
    s_recovery_actual_columns = 0;
    if (s_recovery_repeats != 1U && s_recovery_repeats != 2U) return -4;
    os_printf("[RECOVERY_EXPECT_LINE] stage=%s logical_columns=1180 fire_columns=%lu wire_crc32=%08lX repeats=%u\r\n",
              kind, (unsigned long)(PRINTER_PAGE_COLUMNS * s_recovery_repeats),
              (unsigned long)s_recovery_line_crc[s_recovery_repeats - 1U], s_recovery_repeats);
#endif
    s_idle_ab_first_ack_us = s_idle_ab_last_ack_us = 0;
    os_printf("[IDLE_AB_LINE_BEGIN] phase=%s kind=%s line=%u/3 crc32=%08lX head=ON boost=ON\r\n",
              phase, kind, copy, (unsigned long)crc);
    uint64_t begin = bk_aon_rtc_get_us();
    if (!carriage_stream_move_ramped(1, SHUTTLE_TOTAL_STEPS, columns)) return -9;
    if (s_blank_spi_columns != PRINTER_PAGE_COLUMNS || !s_idle_ab_first_ack_us ||
        s_idle_ab_last_ack_us < s_idle_ab_first_ack_us) return -9;
#if PRINTER_STARTUP_RECOVERY
    if (s_recovery_actual_columns != PRINTER_PAGE_COLUMNS * s_recovery_repeats) return -9;
    os_printf("[RECOVERY_SENT_LINE] stage=%s logical_columns=%lu fire_columns=%lu repeats=%u ACK_only_ink_unverified\r\n",
              kind, (unsigned long)s_blank_spi_columns, (unsigned long)s_recovery_actual_columns, s_recovery_repeats);
#endif
    *lead_us = s_idle_ab_first_ack_us - begin;
    uint32_t gap_ms = idle_anchor ? (uint32_t)((s_idle_ab_first_ack_us - idle_anchor) / 1000U) : 0U;
    rtos_delay_milliseconds(200);
    if (!carriage_stream_move_ramped(0, SHUTTLE_TOTAL_STEPS, NULL)) return -9;
    rtos_delay_milliseconds(300);
    if (printer_crc32(columns, PRINTER_IDLE_AB_BYTES) != crc) return -11;
    if (s_blank_spi_steps[0] != SHUTTLE_TOTAL_STEPS ||
        s_blank_spi_steps[1] != SHUTTLE_TOTAL_STEPS) return -9;
    os_printf("[IDLE_AB_LINE_END] phase=%s kind=%s line=%u/3 columns=%lu/1180 lead_ms=%lu active_gap_ms=%lu crc32=%08lX ink_unverified\r\n",
              phase, kind, copy, (unsigned long)s_blank_spi_columns,
              (unsigned long)(*lead_us / 1000U), (unsigned long)gap_ms, (unsigned long)crc);
    if (idle_anchor && (gap_ms < PRINTER_IDLE_AB_GAP_MS - 100U || gap_ms > PRINTER_IDLE_AB_GAP_MS + 100U)) {
        os_printf("[IDLE_AB_INVALID] phase=%s actual_gap_outside_target target_ms=%lu tolerance_ms=100\r\n", phase, (unsigned long)PRINTER_IDLE_AB_GAP_MS);
        return -12;
    }
    return 0;
}

static int printer_idle_ab_feed(uint32_t steps, const char *phase, unsigned line) {
    uint32_t sent = 0;
    int result = motor_page_feed_keep_power(steps, PRINTER_PAGE_FEED_DIRECTION, &sent);
    os_printf("[IDLE_AB_FEED] phase=%s after=%u pulses=%lu/%lu result=%d head=ON boost_not_toggled distance_unverified\r\n",
              phase, line, (unsigned long)sent, (unsigned long)steps, result);
    if (result != BK_OK || sent != steps) return -10;
    carriage_wake_on_powered_rail();
    return 0;
}

static int printer_idle_ab_phase(const uint8_t *columns, uint32_t crc, unsigned power_off) {
    const char *phase = PRINTER_STARTUP_RECOVERY ? "STARTUP_RECOVERY" : (PRINTER_IDLE_REFERENCE_ONLY ? "CMY_SHORT" : (power_off ? "A_OFF_RESTART" : "B_HELD_POWER"));
    int result;
    uint64_t lead_us = 0;
    os_printf("[IDLE_AB_PHASE] phase=%s reference=3 observe=%u same_bytes_same_nozzles\r\n", phase, PRINTER_IDLE_REFERENCE_ONLY ? 0U : 3U);
    if (tph_mcu_set_head_power(1) != BK_OK) return -8;
    rtos_delay_milliseconds(PRINTER_PAGE_HEAD_SETTLE_MS);
    for (unsigned line = 1; line <= 3U; ++line) {
#if PRINTER_STARTUP_RECOVERY
        static const char *const stages[3] = {"BEFORE", "DOSE_X2", "AFTER"};
        s_recovery_repeats = line == 2U ? 2U : 1U;
        result = printer_idle_ab_scan(columns, crc, phase, stages[line - 1U], line, 0, &lead_us);
#else
        result = printer_idle_ab_scan(columns, crc, phase, "REF", line, 0, &lead_us);
#endif
        if (result) return result;
        result = printer_idle_ab_feed(PRINTER_IDLE_REFERENCE_ONLY && line == 3U ? PRINTER_IDLE_AB_GROUP_FEED : PRINTER_IDLE_AB_LINE_FEED, phase, line);
        if (result) return result;
    }
    if (PRINTER_IDLE_REFERENCE_ONLY) {
        if (tph_mcu_set_head_power(0) != BK_OK) return -8;
        os_printf("[IDLE_AB_PHASE_DONE] phase=%s scans=3/3 no_idle_no_priming ink_unverified\r\n", phase);
        return 0;
    }
    /* Last acknowledged nonzero column in REF3, not end-of-return or end-of-feed.
     * Estimate the forward lead using REF3 so first active ACK lands at the configured interval.
     */
    uint64_t anchor = s_idle_ab_last_ack_us;
    if (lead_us >= PRINTER_IDLE_AB_GAP_US) return -12;
    uint64_t scan_deadline = anchor + PRINTER_IDLE_AB_GAP_US - lead_us;
    uint64_t ready_margin = ((uint64_t)PRINTER_PAGE_HEAD_SETTLE_MS + 300U) * 1000U;
    if (bk_aon_rtc_get_us() + ready_margin >= scan_deadline) return -12;
    if (power_off) {
        if (tph_mcu_set_head_power(0) != BK_OK) return -8;
        carriage_verified_power_down();
        printer_idle_ab_wait_until(scan_deadline - ready_margin, phase, "head_OFF_boost_OFF");
        carriage_verified_power_up();
        if (tph_mcu_set_head_power(1) != BK_OK) return -8;
        rtos_delay_milliseconds(PRINTER_PAGE_HEAD_SETTLE_MS);
    } else {
        printer_idle_ab_park_carriage();
        printer_idle_ab_wait_until(scan_deadline - 100000U, phase, "head_ON_boost_ON_Ax0_Dx0");
        carriage_wake_on_powered_rail();
    }
    printer_idle_ab_wait_until(scan_deadline, phase, "head_ON_ready_Ax0_Dx0");
    uint64_t now = bk_aon_rtc_get_us();
    if (now > scan_deadline + 50000U) return -12;
    os_printf("[IDLE_AB_RESUME] phase=%s estimated_active_gap_ms=%lu lateness_ms=%lu no_preprint_or_priming\r\n",
              phase, (unsigned long)PRINTER_IDLE_AB_GAP_MS, (unsigned long)((now - scan_deadline) / 1000U));
    for (unsigned line = 1; line <= 3U; ++line) {
        result = printer_idle_ab_scan(columns, crc, phase, "OBS", line,
                                     line == 1U ? anchor : 0, &lead_us);
        if (result) return result;
        result = printer_idle_ab_feed(line < 3U ? PRINTER_IDLE_AB_LINE_FEED : PRINTER_IDLE_AB_GROUP_FEED,
                                      phase, line + 3U);
        if (result) return result;
    }
    if (tph_mcu_set_head_power(0) != BK_OK) return -8;
    /* Leave boost ON between phases; phase A itself toggles it during its gap. */
    os_printf("[IDLE_AB_PHASE_DONE] phase=%s scans=6/6 ink_unverified\r\n", phase);
    return 0;
}

static int printer_run_idle_ab(const uint8_t *bin, uint32_t length) {
    static bool failure_locked = false;
    static uint8_t *frozen = NULL;
    static uint32_t frozen_crc, completed;
    if (failure_locked || !printer_link_is_ready()) return -7;
    printer_map_config_t cfg;
    printer_map_default_config(&cfg);
    cfg.x_pitch_centi_um = PRINTER_MAP_X_PITCH_CENTI_UM;
    cfg.x_direction = PRINTER_MAP_X_DIRECTION;
    cfg.first_nozzle = PRINTER_MAP_FIRST_NOZZLE;
    cfg.reverse_y = PRINTER_MAP_REVERSE_Y;
    printer_page_t page;
    int result = printer_page_prepare(bin, length, &cfg, &page,
                           printer_map_psram_alloc, printer_map_psram_free, printer_map_yield_cpu);
    if (result) return result;
    uint8_t *candidate = NULL, *scratch = NULL;
    unsigned reuse = frozen != NULL;
    if (!frozen) {
#if PRINTER_FIXED_S2_BASELINE
        candidate = printer_map_psram_alloc(PRINTER_IDLE_AB_BYTES);
        if (!candidate) { result = -2; goto done; }
        if (printer_fixed_s2_build(candidate, PRINTER_IDLE_AB_BYTES)) { result = -4; goto done; }
#else
        if (page.swaths <= 4U) { result = -4; goto done; }
        result = printer_page_transform(&page, PRINTER_PAGE_TRANSFORM, printer_map_yield_cpu);
        if (result) goto done;
        candidate = printer_map_psram_alloc(PRINTER_IDLE_AB_BYTES);
        scratch = printer_map_psram_alloc(page.width * 72U);
        if (!candidate || !scratch) { result = -2; goto done; }
        uint32_t dots[3];
        result = printer_page_render(&page, 4U, candidate, PRINTER_IDLE_AB_BYTES,
                                     scratch, page.width * 72U, dots, printer_map_yield_cpu);
        if (result) goto done;
#endif
        unsigned first = PRINTER_PAGE_COLUMNS, last = 0, active = 0;
        for (unsigned x = 0; x < PRINTER_PAGE_COLUMNS; ++x) {
            unsigned nonzero = 0;
            for (unsigned slot = 0; slot < 100U; ++slot) nonzero |= candidate[x * 100U + slot];
            if (nonzero) { if (!active) first = x; last = x; ++active; }
        }
#if PRINTER_FIXED_S2_BASELINE
        if (active != PRINTER_FIXED_S2_ACTIVE_COLUMNS) { result = -4; goto done; }
#else
        if (active < 256U || !dots[0] || !dots[1] || !dots[2]) { result = -4; goto done; }
#endif
        s_idle_ab_first_col = first; s_idle_ab_last_col = last;
        frozen = candidate; candidate = NULL;
        frozen_crc = printer_crc32(frozen, PRINTER_IDLE_AB_BYTES);
    }
    printer_page_release(&page, printer_map_psram_free);
    if (scratch) { printer_map_psram_free(scratch); scratch = NULL; }
    if (printer_crc32(frozen, PRINTER_IDLE_AB_BYTES) != frozen_crc) {
        failure_locked = true; result = -11; goto done;
    }
#if PRINTER_STARTUP_RECOVERY
    s_recovery_repeats = 1U;
    s_recovery_line_crc[0] = frozen_crc;
    s_recovery_line_crc[1] = printer_recovery_crc_update(0xFFFFFFFFU, frozen, 2U) ^ 0xFFFFFFFFU;
    uint32_t job_crc = printer_recovery_crc_update(0xFFFFFFFFU, frozen, 1U);
    job_crc = printer_recovery_crc_update(job_crc, frozen, 2U);
    s_recovery_job_crc = printer_recovery_crc_update(job_crc, frozen, 1U) ^ 0xFFFFFFFFU;
    os_printf("[RECOVERY_BEGIN] stages=BEFORE,DOSE_X2,AFTER scans=3 head_sessions=1 no_wipe_no_reseat same_waveform\r\n");
    os_printf("[RECOVERY_RX_EXPECT] columns=4720 bytes=472000 crc32=%08lX repeat_limit=2 HC_CONT_CE_v1_60us_idle_guard_required\r\n",
              (unsigned long)s_recovery_job_crc);
#endif
    os_printf("[IDLE_AB_BEGIN] run=%lu order=%s cache=%s crc32=%08lX scans=%u first_active=%lu last_active=%lu target_gap_ms=%lu HC_CONT_CE_required\r\n",
              (unsigned long)(completed + 1U), PRINTER_IDLE_REFERENCE_ONLY ? "REFERENCE_ONLY" : (PRINTER_IDLE_OFF_ONLY ? "OFF_ONLY" : (PRINTER_IDLE_HELD_ONLY ? "HELD_ONLY" : ((completed & 1U) ? "BA" : "AB"))), reuse ? "REUSED" : "CAPTURED",
              (unsigned long)frozen_crc, PRINTER_IDLE_REFERENCE_ONLY ? 3U : (PRINTER_IDLE_SINGLE_PHASE ? 6U : 12U),
              (unsigned long)s_idle_ab_first_col, (unsigned long)s_idle_ab_last_col, (unsigned long)(PRINTER_IDLE_REFERENCE_ONLY ? 0U : PRINTER_IDLE_AB_GAP_MS));
#if PRINTER_FIXED_S2_BASELINE
    if (frozen_crc != PRINTER_FIXED_S2_CRC32) { failure_locked = true; result = -11; goto done; }
    os_printf("[FIXED_S2] v1 A=004A2912 S=02 D=3F frame_crc32=%08lX active_columns=%u per_nozzle_per_frame=711 per_nozzle_per_job=2844 selected_nozzles=24 APP_pixels_ignored fixed_across_boots\r\n",
              (unsigned long)frozen_crc, PRINTER_FIXED_S2_ACTIVE_COLUMNS);
#endif
    failure_locked = true;
    if (tph_mcu_set_head_power(0) != BK_OK) { result = -8; goto stop; }
    carriage_verified_power_up();
    for (unsigned phase = 0; phase < (PRINTER_IDLE_SINGLE_PHASE ? 1U : 2U); ++phase) {
        unsigned is_a = PRINTER_IDLE_OFF_ONLY ? 1U : (PRINTER_IDLE_HELD_ONLY ? 0U : ((phase ^ (completed & 1U)) == 0U));
        result = printer_idle_ab_phase(frozen, frozen_crc, is_a);
        if (result) goto stop;
    }
stop:
#if PRINTER_STARTUP_RECOVERY
    s_recovery_repeats = 1U;
#endif
    if (tph_mcu_set_head_power(0) != BK_OK && !result) result = -8;
    bk_motor_set_state(0, 0);
    carriage_verified_power_down();
    if (!result) { failure_locked = false; ++completed; }
    os_printf("[IDLE_AB_END] code=%d completed_runs=%lu failure_locked=%u head_OFF_requested boost_OFF ink_unverified\r\n",
              result, (unsigned long)completed, (unsigned)failure_locked);
done:
    if (scratch) printer_map_psram_free(scratch);
    if (candidate) printer_map_psram_free(candidate);
    printer_page_release(&page, printer_map_psram_free);
    if (result && failure_locked && frozen) { printer_map_psram_free(frozen); frozen = NULL; }
    return result;
}
