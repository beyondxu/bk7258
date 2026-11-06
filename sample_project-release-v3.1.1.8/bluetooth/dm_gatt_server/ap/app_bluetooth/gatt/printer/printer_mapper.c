#include "printer_mapper.h"
#include "printer_audit_utils.h"
#include <string.h>
#include "printer_nozzle_table.h"

#define MAP_MAX_WIDTH 4096U
#define MAP_MAX_Y 8192U
#define MAP_MAX_DECODED (2U * 1024U * 1024U)

void printer_map_default_config(printer_map_config_t *cfg)
{
    if (cfg) {
        cfg->x_pitch_centi_um = 4233;
        cfg->x_direction = 1;
        cfg->first_nozzle = 0;
        cfg->reverse_y = 0;
    }
}
static int config_valid(const printer_map_config_t *cfg)
{
    return cfg && cfg->x_pitch_centi_um >= 1000 && cfg->x_pitch_centi_um <= 100000 &&
           (cfg->x_direction == 1 || cfg->x_direction == -1) &&
           cfg->first_nozzle <= 8 && cfg->reverse_y <= 1;
}
static uint32_t domain_offset(uint32_t bit, const printer_map_config_t *cfg)
{
    return (map_domain_x[bit] - map_domain_x[5] + cfg->x_pitch_centi_um / 2) / cfg->x_pitch_centi_um;
}
int printer_map_route(uint32_t channel, uint32_t row, const printer_map_config_t *cfg,
                      printer_map_route_t *route)
{
    if (!config_valid(cfg) || !route || channel >= 3 || row >= PRINTER_MAP_ROWS) return -1;
    uint32_t nozzle = cfg->first_nozzle + row;
    if (cfg->reverse_y) nozzle = 199U - nozzle;
    uint32_t bit = channel * 2U + map_row_pair_bit[nozzle];
    uint32_t offset = domain_offset(bit, cfg);
    route->slot = map_row_slot[nozzle];
    route->mask = 1U << bit;
    route->delay_columns = cfg->x_direction == 1 ? domain_offset(0, cfg) - offset : offset;
    route->nozzle_row = nozzle;
    return 0;
}
static int make_column(const uint8_t *planes, uint32_t width, uint32_t column,
                       const printer_map_config_t *cfg, uint8_t out[100])
{
    memset(out, 0, 100);
    for (uint32_t channel = 0; channel < 3; ++channel) {
        for (uint32_t row = 0; row < PRINTER_MAP_ROWS; ++row) {
            printer_map_route_t route;
            if (printer_map_route(channel, row, cfg, &route)) return -1;
            if (column < route.delay_columns) continue;
            uint32_t source_x = column - route.delay_columns;
            if (source_x >= width) continue;
            uint32_t offset = channel * width * 24U + source_x * 24U + row / 8U;
            if (planes[offset] & (0x80U >> (row % 8U))) out[route.slot] |= (uint8_t)route.mask;
        }
    }
    return 0;
}
int printer_map_column(const uint8_t *planes, uint32_t width, uint32_t column,
                       const printer_map_config_t *cfg, uint8_t out[100])
{
    if (!planes || !out || !config_valid(cfg) || !width || width > MAP_MAX_WIDTH) return -1;
    if (column >= width + domain_offset(0, cfg)) return -1;
    return make_column(planes, width, column, cfg, out);
}

typedef struct {
    const uint8_t *data;
    uint32_t length, pos, decoded, run;
    uint8_t compressed, repeated, value;
} map_reader_t;
static int read_byte(map_reader_t *r)
{
    if (!r->compressed) {
        if (r->pos == r->length) return -1;
        if (r->decoded >= MAP_MAX_DECODED) return -2;
        ++r->decoded;
        return r->data[r->pos++];
    }
    while (!r->run) {
        if (r->pos == r->length) return -1;
        uint8_t flag = r->data[r->pos++];
        if (flag <= 0x7E) {
            r->run = (uint32_t)flag + 1;
            r->repeated = 0;
            if (r->run > r->length - r->pos) return -2;
        } else if (flag >= 0x82) {
            if (r->pos == r->length) return -2;
            r->run = 257U - flag;
            r->value = r->data[r->pos++];
            r->repeated = 1;
        }
        /* Native decoder ignores 0x7F/0x80/0x81, verified against APK .so. */
    }
    if (r->decoded >= MAP_MAX_DECODED) return -2;
    --r->run;
    ++r->decoded;
    return r->repeated ? r->value : r->data[r->pos++];
}
static unsigned bit_count(uint8_t n)
{
    unsigned count = 0;
    while (n) { count += n & 1U; n >>= 1; }
    return count;
}
static uint32_t le32(uint8_t *p, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(value >> (i * 8));
    return value;
}
static int flush_swath(const uint8_t *planes, uint32_t width, uint32_t y,
                       const printer_map_config_t *cfg, printer_map_stats_t *stats,
                       uint32_t *crc, void (*yield_cpu)(void), printer_map_capture_t *capture)
{
    uint32_t delay = domain_offset(0, cfg);
    uint8_t column[100], position[8], allowed[100] = {0};
    int taking = capture && capture->target_y == y;
    if (taking) {
        if (capture->found) return -5;
        capture->found = 1;
        for (uint32_t ch = 0; ch < 3; ++ch) {
            if (!(capture->channel_mask & (1U << ch))) continue;
            for (uint32_t row = capture->first_row; row <= capture->last_row; ++row) {
                printer_map_route_t route;
                if (printer_map_route(ch, row, cfg, &route)) return -1;
                allowed[route.slot] |= (uint8_t)route.mask;
            }
        }
    }
    for (uint32_t channel = 0; channel < 3; ++channel) {
        const uint8_t *p = planes + channel * width * 24U;
        for (uint32_t i = 0; i < width * 24U; ++i) stats->input_dots[channel] += bit_count(p[i]);
    }
    for (uint32_t x = 0; x < width + delay; ++x) {
        if (yield_cpu && (x & 31U) == 0) yield_cpu();
        if (make_column(planes, width, x, cfg, column)) return -1;
        if (taking) {
            for (uint32_t slot = 0; slot < 100; ++slot) {
                uint8_t bits = column[slot] & allowed[slot];
                if (bits) {
                    if (x >= capture->column_limit || x < capture->min_column || x > capture->max_column) return -5;
                    capture->columns[x * 100U + slot] = bits;
                    if (x < capture->first_active) capture->first_active = x;
                    if (x > capture->last_active) capture->last_active = x;
                    for (uint32_t bit = 0; bit < 6; ++bit) capture->selected_dots[bit / 2U] += (bits >> bit) & 1U;
                }
            }
        }
        le32(position, y); le32(position + 4, x);
        *crc = printer_crc32_update(*crc, position, 8);
        *crc = printer_crc32_update(*crc, column, 100);
        for (uint32_t slot = 0; slot < 100; ++slot) {
            for (uint32_t bit = 0; bit < 6; ++bit) stats->output_dots[bit / 2U] += (column[slot] >> bit) & 1U;
        }
    }
    ++stats->swaths;
    stats->output_columns += width + delay;
    if (width > stats->max_width) stats->max_width = width;
    stats->max_delay = delay;
    return 0;
}
static int dispatch_swath(const uint8_t *planes, uint32_t width, uint32_t y,
                          const printer_map_config_t *cfg, printer_map_stats_t *stats,
                          uint32_t *crc, void (*yield_cpu)(void), printer_map_capture_t *capture,
                          printer_map_visitor_t visitor, void *context)
{
    if (!visitor) return flush_swath(planes, width, y, cfg, stats, crc, yield_cpu, capture);
    int result = visitor(planes, width, y, context);
    if (result) return result;
    ++stats->swaths;
    if (width > stats->max_width) stats->max_width = width;
    if (yield_cpu) yield_cpu();
    return 0;
}
static int map_bin_run(const uint8_t *bin, uint32_t length,
                           const printer_map_config_t *cfg, printer_map_stats_t *stats,
                           void *(*allocate)(size_t), void (*release)(void *), void (*yield_cpu)(void), printer_map_capture_t *capture,
                           printer_map_visitor_t visitor, void *context)
{
    if (!bin || !length || !stats || !allocate || !release || !config_valid(cfg)) return -1;
    memset(stats, 0, sizeof(*stats));
    uint32_t offset = 0;
    if (length >= 4 && !memcmp(bin, "\x1b\x10\x03\x04", 4)) {
        if (length < 25 || memcmp(bin + 5, "\x1b\x10\x03\x05", 4)) return -3;
        if (bin[4] != 1) return -4;
        offset = 25;
    }
    int compressed = length - offset >= 4 && !memcmp(bin + offset, "\x1b\x25\x39\x01", 4);
    if (compressed) offset += 4;
    map_reader_t reader = {bin + offset, length - offset, 0, 0, 0, (uint8_t)compressed, 0, 0};
    uint8_t *planes = NULL;
    uint32_t width = 0, seen = 0, channel = 3, y = 0, crc = 0xFFFFFFFFU;
    int result = -3;
    for (;;) {
        int first = read_byte(&reader);
        if (first < 0) break;
        if (first == 12) {
            if (seen) {
                if (seen != 7) break;
                int code = dispatch_swath(planes, width, y, cfg, stats, &crc, yield_cpu, capture, visitor, context);
                if (code) { result = code; break; }
            }
            if (!stats->swaths || read_byte(&reader) != -1) break;
            stats->decoded_bytes = reader.decoded;
            stats->columns_crc32 = crc ^ 0xFFFFFFFFU;
            result = 0;
            break;
        }
        if (first != 0x1B) break;
        int second = read_byte(&reader);
        if (second == 0x56) {
            int value = read_byte(&reader);
            if (value < 1 || value > 3) { result = -4; break; }
            channel = (uint32_t)value - 1;
            continue;
        }
        if (second == 0x0D) {
            int dpi = read_byte(&reader);
            if (dpi < 1 || dpi > 2) { result = -4; break; }
            continue;
        }
        if (second != 0x25) { result = -4; break; }
        int command = read_byte(&reader);
        if (command == 0x36) {
            int high = read_byte(&reader), low = read_byte(&reader);
            if (high < 0 || low < 0) break;
            if (high || low) { result = -4; break; }
            continue;
        }
        if (command == 0x35) {
            int feed = read_byte(&reader);
            if (feed < 0) break;
            if (feed != 192) { result = -4; break; }
            if (seen != 7) break;
            int code = dispatch_swath(planes, width, y, cfg, stats, &crc, yield_cpu, capture, visitor, context);
            if (code) { result = code; break; }
            seen = 0;
            channel = 3;
            y += (uint32_t)feed;
            if (y + 192 > MAP_MAX_Y) { result = -4; break; }
            continue;
        }
        if (command != 0x32) { result = -4; break; }
        int high = read_byte(&reader), low = read_byte(&reader);
        if (high < 0 || low < 0) break;
        uint32_t next_width = (uint32_t)high * 256U + (uint32_t)low;
        if (!next_width || next_width > MAP_MAX_WIDTH || channel >= 3) { result = -4; break; }
        if (seen & (1U << channel)) break;
        if (seen && next_width != width) { result = -4; break; }
        if (!planes || next_width != width) {
            if (planes) release(planes);
            planes = allocate((size_t)next_width * 72U);
            if (!planes) { result = -2; break; }
        }
        width = next_width;
        uint8_t *p = planes + channel * width * 24U;
        uint32_t n = width * 24U;
        while (n--) {
            int byte = read_byte(&reader);
            if (byte < 0) goto done;
            *p++ = (uint8_t)byte;
        }
        seen |= 1U << channel;
    }
done:
    if (planes) release(planes);
    if (!result) {
        for (unsigned i = 0; i < 3; ++i) {
            if (stats->input_dots[i] != stats->output_dots[i]) return -3;
        }
    }
    return result;
}

int printer_map_bin_dry_run(const uint8_t *bin, uint32_t length,
                           const printer_map_config_t *cfg, printer_map_stats_t *stats,
                           void *(*allocate)(size_t), void (*release)(void *), void (*yield_cpu)(void))
{
    return map_bin_run(bin, length, cfg, stats, allocate, release, yield_cpu, NULL, NULL, NULL);
}
int printer_map_bin_capture(const uint8_t *bin, uint32_t length,
                           const printer_map_config_t *cfg, printer_map_stats_t *stats,
                           printer_map_capture_t *capture,
                           void *(*allocate)(size_t), void (*release)(void *), void (*yield_cpu)(void))
{
    if (!capture || !capture->columns || !capture->column_limit || capture->column_limit > MAP_MAX_WIDTH ||
        capture->buffer_bytes < capture->column_limit * 100U || capture->target_y % 192U ||
        capture->first_row > capture->last_row || capture->last_row >= 192 ||
        !capture->channel_mask || (capture->channel_mask & ~7U) ||
        capture->min_column > capture->max_column || capture->max_column >= capture->column_limit) return -1;
    memset(capture->columns, 0, capture->column_limit * 100U);
    capture->found = 0;
    memset(capture->selected_dots, 0, sizeof(capture->selected_dots));
    capture->first_active = UINT32_MAX; capture->last_active = 0;
    int result = map_bin_run(bin, length, cfg, stats, allocate, release, yield_cpu, capture, NULL, NULL);
    if (!result && (!capture->found || capture->first_active == UINT32_MAX)) result = -5;
    return result;
}

int printer_map_solid_line(uint8_t *columns, uint32_t buffer_bytes,
                          uint32_t column_count, uint32_t first, uint32_t last,
                          uint32_t channel, uint32_t row,
                          const printer_map_config_t *cfg)
{
    printer_map_route_t route;
    if (!columns || !column_count || column_count > MAP_MAX_WIDTH ||
        buffer_bytes < column_count * 100U || first > last || last >= column_count ||
        printer_map_route(channel, row, cfg, &route)) return -1;
    memset(columns, 0, column_count * 100U);
    for (uint32_t x = first; x <= last; ++x)
        columns[x * 100U + route.slot] = (uint8_t)route.mask;
    return 0;
}

static int map_registration_rows(uint8_t *columns, uint32_t buffer_bytes,
                                  uint32_t column_count, uint32_t channel_mask,
                                  const uint32_t rows[3], const printer_map_config_t *cfg)
{
    printer_map_route_t routes[3];
    if (!columns || !column_count || column_count > MAP_MAX_WIDTH ||
        buffer_bytes < column_count * 100U || !channel_mask || (channel_mask & ~7U)) return -1;
    for (uint32_t ch = 0; ch < 3; ++ch) {
        if (!(channel_mask & (1U << ch))) continue;
        if (printer_map_route(ch, rows[ch], cfg, &routes[ch]) ||
            849U + routes[ch].delay_columns >= column_count ||
            849U + routes[ch].delay_columns > 999U) return -1;
    }
    memset(columns, 0, column_count * 100U);
    for (uint32_t ch = 0; ch < 3; ++ch) {
        if (!(channel_mask & (1U << ch))) continue;
        for (uint32_t bar = 0; bar < 3; ++bar) {
            for (uint32_t x = 300U + bar * 200U; x < 450U + bar * 200U; ++x) {
                uint32_t output_x = x + routes[ch].delay_columns;
                columns[output_x * 100U + routes[ch].slot] |= (uint8_t)routes[ch].mask;
            }
        }
    }
    return 0;
}

int printer_map_alignment_pattern(uint8_t *columns, uint32_t buffer_bytes,
                                  uint32_t column_count, uint32_t channel_mask,
                                  uint32_t row, const printer_map_config_t *cfg)
{
    const uint32_t rows[3] = {row, row, row};
    return map_registration_rows(columns, buffer_bytes, column_count, channel_mask, rows, cfg);
}
int printer_map_color_rows_pattern(uint8_t *columns, uint32_t buffer_bytes,
                                  uint32_t column_count, uint32_t channel_mask,
                                  const printer_map_config_t *cfg)
{
    const uint32_t rows[3] = {PRINTER_MAP_TEST_ROW_C, PRINTER_MAP_TEST_ROW_M, PRINTER_MAP_TEST_ROW_Y};
    return map_registration_rows(columns, buffer_bytes, column_count, channel_mask, rows, cfg);
}

int printer_map_visit_bin(const uint8_t *bin, uint32_t length,
                         const printer_map_config_t *cfg, printer_map_stats_t *stats,
                         printer_map_visitor_t visitor, void *context,
                         void *(*allocate)(size_t), void (*release)(void *), void (*yield_cpu)(void))
{
    if (!visitor) return -1;
    return map_bin_run(bin, length, cfg, stats, allocate, release, yield_cpu, NULL, visitor, context);
}

int printer_map_nozzle_lines(uint8_t *columns, uint32_t buffer_bytes,
                             uint32_t column_count, uint32_t channel,
                             const printer_map_config_t *cfg) {
    if (!columns || !config_valid(cfg) || channel >= 3 || column_count != 1180U ||
        buffer_bytes < column_count * 100U) return -1;
    /* Validate every route before altering the caller's output. */
    for (uint32_t row = 0; row < 192U; ++row) {
        printer_map_route_t route;
        if (printer_map_route(channel, row, cfg, &route) ||
            120U + (row % 6U) * 160U + 96U + route.delay_columns > column_count) return -1;
    }
    memset(columns, 0, column_count * 100U);
    for (uint32_t row = 0; row < 192U; ++row) {
        printer_map_route_t route;
        if (printer_map_route(channel, row, cfg, &route)) return -1;
        uint32_t first = 120U + (row % 6U) * 160U + route.delay_columns;
        for (uint32_t x = first; x < first + 96U; ++x)
            columns[x * 100U + route.slot] |= (uint8_t)route.mask;
    }
    return 0;
}
