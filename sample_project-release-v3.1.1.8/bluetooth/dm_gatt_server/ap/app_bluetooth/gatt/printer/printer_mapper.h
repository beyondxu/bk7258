#ifndef PRINTER_MAPPER_H
#define PRINTER_MAPPER_H
#include <stddef.h>
#include <stdint.h>
#define PRINTER_MAP_ROWS 192U
#define PRINTER_MAP_COLUMN_BYTES 100U
/* All settings describe a CANDIDATE physical geometry, not calibrated motion. */
typedef struct {
    uint32_t x_pitch_centi_um; /* default 4233 (42.33um); MUST calibrate before firing */
    int32_t x_direction;       /* +1 increasing physical X, -1 decreasing physical X */
    uint32_t first_nozzle;     /* 0..8 guard-row placement for 192 of 200 nozzles */
    uint32_t reverse_y;        /* 0 descending physical Y, 1 ascending physical Y */
} printer_map_config_t;
typedef struct {
    uint32_t slot;
    uint32_t mask;
    uint32_t delay_columns;
    uint32_t nozzle_row;
} printer_map_route_t;
typedef struct {
    uint32_t swaths;
    uint32_t input_dots[3];
    uint32_t output_dots[3];
    uint32_t output_columns;
    uint32_t max_width;
    uint32_t max_delay;
    uint32_t columns_crc32; /* canonical stream: uint32LE y, uint32LE col, 100B */
    uint32_t decoded_bytes;
} printer_map_stats_t;
typedef struct {
    uint8_t *columns;
    uint32_t buffer_bytes, column_limit;
    uint32_t target_y, first_row, last_row, channel_mask;
    uint32_t min_column, max_column;
    uint32_t found, selected_dots[3], first_active, last_active;
} printer_map_capture_t;
/* Build a complete, bounded diagnostic plan. Never drives hardware.
 * -5 means target/range/blank-plan rejection. Caller discards buffer on failure. */
int printer_map_bin_capture(const uint8_t *bin, uint32_t length,
                           const printer_map_config_t *cfg, printer_map_stats_t *stats,
                           printer_map_capture_t *capture,
                           void *(*allocate)(size_t), void (*release)(void *), void (*yield_cpu)(void));
/* Diagnostic: one physical nozzle, one bit per column, outside range zero. */
int printer_map_solid_line(uint8_t *columns, uint32_t buffer_bytes,
                          uint32_t column_count, uint32_t first, uint32_t last,
                          uint32_t channel, uint32_t row,
                          const printer_map_config_t *cfg);
#define PRINTER_MAP_TEST_ROW_C 95U
#define PRINTER_MAP_TEST_ROW_M 135U
#define PRINTER_MAP_TEST_ROW_Y 175U
int printer_map_color_rows_pattern(uint8_t *columns, uint32_t buffer_bytes,
                                  uint32_t column_count, uint32_t channel_mask,
                                  const printer_map_config_t *cfg);
int printer_map_section_paper_order(uint8_t *columns, uint32_t buffer_bytes,
                                    uint32_t column_count, uint32_t section,
                                    uint32_t reverse_columns,
                                    const printer_map_config_t *cfg);
int printer_map_section_paper(uint8_t *columns, uint32_t buffer_bytes,
                              uint32_t column_count, uint32_t section,
                              const printer_map_config_t *cfg);
int printer_map_section_scope(uint8_t *columns, uint32_t buffer_bytes,
                              uint32_t column_count, uint32_t section,
                              const printer_map_config_t *cfg);
int printer_map_nozzle_lines_batch(uint8_t *columns, uint32_t buffer_bytes,
                                   uint32_t column_count, uint32_t channel,
                                   uint32_t batch, uint32_t batches,
                                   const printer_map_config_t *cfg);
/* 192 used nozzle rows: six interleaved blocks, 96 points per nozzle. */
int printer_map_nozzle_lines(uint8_t *columns, uint32_t buffer_bytes,
                             uint32_t column_count, uint32_t channel,
                             const printer_map_config_t *cfg);
/* Three 150-column bars at logical x=300,500,700 with per-domain delay. */
int printer_map_alignment_pattern(uint8_t *columns, uint32_t buffer_bytes,
                                  uint32_t column_count, uint32_t channel_mask,
                                  uint32_t row, const printer_map_config_t *cfg);
/* Visitor receives complete C/M/Y planes (width*72 bytes) per 192-row strip.
 * Caller must not drive hardware: a later parse error can invalidate prior strips.
 */
typedef int (*printer_map_visitor_t)(const uint8_t *, uint32_t, uint32_t, void *);
int printer_map_visit_bin(const uint8_t *bin, uint32_t length,
                         const printer_map_config_t *cfg, printer_map_stats_t *stats,
                         printer_map_visitor_t visitor, void *context,
                         void *(*allocate)(size_t), void (*release)(void *), void (*yield_cpu)(void));
void printer_map_default_config(printer_map_config_t *cfg);
int printer_map_route(uint32_t channel, uint32_t row, const printer_map_config_t *cfg,
                      printer_map_route_t *route);
int printer_map_column(const uint8_t *planes, uint32_t width, uint32_t column,
                       const printer_map_config_t *cfg, uint8_t out[100]);
/* Receive-only: parses complete native BIN, validates and fingerprints columns.
 * No driver, GPIO or motion calls. Callers provide PSRAM alloc/free on-device.
 * Return 0=success, -1=invalid config/arguments, -2=allocation failure,
 * -3=malformed stream, -4=unsupported format; partial stats are NOT a valid job.
 */
int printer_map_bin_dry_run(const uint8_t *bin, uint32_t length,
                           const printer_map_config_t *cfg, printer_map_stats_t *stats,
                           void *(*allocate)(size_t), void (*release)(void *), void (*yield_cpu)(void));
#endif
