/* Private implementation included by printer_protocol.c after its job/status
 * declarations. Audit mode consumes EXACTLY the announced BIN byte count;
 * binary bytes are never searched for control-frame signatures.
 */
static uint8_t audit_frame[128];
static uint32_t audit_frame_used;

static uint16_t audit_u16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}
static uint32_t audit_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static void audit_send_finish(uint16_t id, uint8_t result)
{
    uint8_t params[9] = {1, 2, 0, (uint8_t)id, (uint8_t)(id >> 8), 2, 1, 0, result};
    send_dn20_response(6, 4, 2, params, sizeof(params));
}
static void audit_command(const uint8_t *frame, uint32_t len)
{
    if (len < 14 || frame[len - 1] != FRAME_TAIL || frame[6] != 1 ||
        audit_u16(frame + 2) != audit_u16(frame + 7) + 5U ||
        len != audit_u16(frame + 7) + 14U) {
        s_status.fault |= 8;
        os_printf("[AUDIT_ERROR] malformed control frame\r\n");
        return;
    }
    uint16_t count = audit_u16(frame + 7);
    const uint8_t *p = frame + 9;
    if (frame[4] == 6 && frame[5] == 3) {
        uint32_t off = 0, length = 0, crc = 0;
        uint16_t id = 0;
        unsigned seen = 0;
        bool valid = true;
        while (off < count) {
            if (count - off < 3) { valid = false; break; }
            uint8_t tag = p[off];
            uint16_t n = audit_u16(p + off + 1);
            off += 3;
            if (n > count - off) { valid = false; break; }
            if (tag == 1) {
                if (n != 2 || (seen & 1)) { valid = false; break; }
                id = audit_u16(p + off); seen |= 1;
            } else if (tag == 2) {
                if (n != 4 || (seen & 2)) { valid = false; break; }
                length = audit_u32(p + off); seen |= 2;
            } else if (tag == 3) {
                if (n != 4 || (seen & 4)) { valid = false; break; }
                crc = audit_u32(p + off); seen |= 4;
            }
            off += n;
        }
        uint8_t refused = 1;
        if (valid && seen == 7 && length && length <= 4U * 1024U * 1024U &&
            s_job.state == JOB_STATE_IDLE && hw_printer_prepare_buffer(length)) {
            memset(&s_job, 0, sizeof(s_job));
            s_job.task_id = id;
            s_job.total_bin_len = length;
            s_job.expected_crc = crc;
            s_job.running_crc = 0xFFFFFFFFU;
            s_job.state = JOB_STATE_RECEIVING_BIN;
            s_status.work = 1;
            s_status.fault = 0;
            refused = 0;
            os_printf("[AUDIT_START] task=%u expected_len=%lu expected_crc32=%08lX\r\n",
                      id, (unsigned long)length, (unsigned long)crc);
        } else {
            os_printf("[AUDIT_ERROR] start rejected: invalid metadata, busy or allocation failed\r\n");
        }
        send_dn20_response(6, 3, 2, &refused, 1);
        return;
    }
    if (frame[4] == 6 && frame[5] == 4) {
        uint16_t id = count == 2 ? audit_u16(p) : s_job.task_id;
        if (s_job.state == JOB_STATE_PRINTING) {
            audit_send_finish(id, count == 2 && id == s_job.task_id ? 2 : 1);
            return;
        }
        bool ok = count == 2 && id == s_job.task_id && s_job.state == JOB_STATE_RECEIVING_BIN &&
                  s_job.received_bin_len == s_job.total_bin_len &&
                  (s_job.running_crc ^ 0xFFFFFFFFU) == s_job.expected_crc;
        os_printf("[AUDIT_RX] task=%u received=%lu expected=%lu crc32=%08lX expected_crc32=%08lX %s\r\n",
                  id, (unsigned long)s_job.received_bin_len, (unsigned long)s_job.total_bin_len,
                  (unsigned long)(s_job.running_crc ^ 0xFFFFFFFFU),
                  (unsigned long)s_job.expected_crc, ok ? "PASS" : "FAIL");
        if (!ok) {
            audit_send_finish(id, 1);
            hw_printer_clear_buffer();
            s_job.state = JOB_STATE_IDLE;
            s_status.work = 0;
            s_status.fault |= 8;
            return;
        }
        s_job.state = JOB_STATE_PRINTING; /* worker owns buffer until audit ends */
        audit_send_finish(id, 2);
        hw_printer_start_job_async();
        return;
    }
    /* Length-sensitive legacy setters need their declared parameter count. */
    if (frame[4] == 6 && (frame[5] == 6 || frame[5] == 7) && count != 1) return;
    if (frame[4] == 6 && frame[5] == 9 && count != 2) return;
    /* Legacy handlers below only answer queries/setters; no job is dispatched. */
    printer_process_data_legacy(frame, len);
}
static void printer_audit_feed(const uint8_t *data, uint32_t len)
{
    if (!data || !len) return;
    while (len) {
        if (s_job.state == JOB_STATE_RECEIVING_BIN && s_job.received_bin_len < s_job.total_bin_len) {
            uint32_t n = s_job.total_bin_len - s_job.received_bin_len;
            if (n > len) n = len;
            if (!hw_printer_append_data(data, n)) {
                os_printf("[AUDIT_ERROR] buffer append rejected; job aborted\r\n");
                hw_printer_clear_buffer();
                s_job.state = JOB_STATE_IDLE;
                s_status.work = 0;
                s_status.fault |= 8;
                audit_frame_used = 0;
                return;
            }
            s_job.running_crc = printer_crc32_update(s_job.running_crc, data, n);
            s_job.received_bin_len += n;
            ++s_job.packet_count;
            data += n; len -= n;
            continue;
        }
        uint8_t byte = *data++; --len;
        if (!audit_frame_used && byte != FRAME_HEAD) continue;
        audit_frame[audit_frame_used++] = byte;
        if (audit_frame_used >= 4) {
            uint32_t need = audit_u16(audit_frame + 2) + 9U;
            if (need < 14 || need > sizeof(audit_frame)) {
                audit_frame_used = 0;
                s_status.fault |= 8;
                os_printf("[AUDIT_ERROR] invalid control length\r\n");
            } else if (audit_frame_used == need) {
                /* Reset before dispatch, so a task transition cannot retain a frame. */
                audit_frame_used = 0;
                audit_command(audit_frame, need);
            }
        }
    }
}
