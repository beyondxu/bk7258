# Inspect actual APP BIN rather than the fixed inspection recipe

The user's APP differs from the sample printer APP. Existing AUDIT_RX logs
prove received length/CRC only; they do not show color pixels or nozzle choice.
The prior S1 12-line inspection explicitly ignored APP pixels. Its D5/D6 counts
are not evidence of APP yellow data. There is no raw BIN available in those logs.

## This firmware

PRINTER_APP_PAYLOAD_AUDIT=1 routes the worker before every printing branch.
Startup does not initialize SPI, run the HC32 handshake or motor/head diagnostics,
and leaves boost disabled. HC32 remains on its existing firmware; waiting for
self-test is expected in this receive-only mode. Normal APP transport/receive
CRC checks remain active. The actual BIN is parsed and rendered in memory only;
there is no generated fixed pattern, SPI, head power command, paper/carriage
motion or firing. MS3 remains GPIO17 for later normal printing.

Send the same C/M/Y test image from the user's APP once. No wipe, reseat or
artificial idle is needed. Save the complete BK UART log, not just HC32 lines or
the CRC summary. The startup banner must be APP_PAYLOAD_AUDIT_V1. A complete
recent 40KB BIN takes about 13 seconds just for paced UART export, plus parsing.
Wait for APP_PAYLOAD_RESULT. APP completion means audit completion, not printing.

## Evidence

APP_BIN_BEGIN/HEX/END export exact received BIN bytes with offsets and CRC.
Export is paced at one 64-byte chunk per 20ms for 115200-baud capture. At most
131072 bytes are exported; larger inputs explicitly set complete=0 but are still
analyzed in full. Never interpret a truncated/missing chunk export as exact data.
APP_CHANNEL gives source and resized pixel counts, IDs 1/2/3 labelled C/M/Y by
the current decoder. These labels are an interpretation to verify against the
known input and native BIN, not proof the APP used the intended color order.
APP_GEOMETRY records layout settings. APP_ROUTE gives actual rendered APP
bit counts and A-index support for all D1..D6 and S1..S4. APP_SELECTED checks
how many of those actual APP bits fall on the current fixed inspection's S1
address/nozzle subset. Zero selected bits can explain a blank APP-selected
subset, but cannot explain the earlier fixed-pattern test, which generated
nonzero data independently of the APP. No statistics prove physical ink output.

Interpretation: missing channel-3 pixels suggests investigating APP content,
color separation or channel-ID interpretation. Nonzero channel-3 source pixels
with lost output require investigating the decoder/layout/mapping. Data routed
to other nozzles can be compared directly to the fixed selected subset. Raw BIN
export enables an independent check rather than trusting receive CRC alone.

## Recover/preview offline

python3 tools/extract_app_bin.py uart.log actual-app.bin
python3 tools/page_preview.py actual-app.bin --out actual-app-preview.png

Extraction validates contiguous offsets, byte count, end marker and CRC, rejecting
truncation and loss. The preview uses the current firmware's layout/decoder;
it is useful evidence but not an independent validation of that decoder.

## Validation

Actual audit C with ASan/UBSan verifies exact compressed raw recovery, all/absent
colors, blank input, malformed streams, all allocation failures and route counts
against resized counts. Oversized raw export is marked incomplete and rejected
by extraction while full analysis succeeds. Tests reject corrupted/missing UART
chunks. Protocol tests preserve true data faults and failed completion. SDK
compiles worker, protocol, motor and startup. Full target build/flash and the
user's actual image capture remain to be done; no APP-origin conclusion yet.

## Task 1059: captured six-line CMYYMC image

45246 received/exported bytes, CRC32 20C67068 matches APP declaration and complete
reconstruction. One serial monitor timestamp split the hex field at offset
0x6FC0 after 47 nibbles. Joining only the UI timestamp continuation recovered
the full byte count and exact CRC. The extraction tool now supports this case
and still rejects missing/corrupt/truncated data. No firmware update is needed
for this offline extraction fix.

Independent Python native decoding agrees with firmware C source counts:
C=35891, M=36001, Y=34951. Main bands in raw source Y coordinates:
C 217..238 and 1140..1162; M 401..423 and 955..977;
Y 585..607 and 771..792. Raw source is 1180x1536 with known padded rows.
There are also 22 isolated Y pixels at rows 289..300 and edge halftoning/scaling
relative to the supplied clean target. These differences do not remove the two
yellow bands. Source color order is CMYYMC, with no evident channel swap.
The firmware resize gives 1119x992 and counts C=23099,M=23373,Y=22724.

Under the current candidate nozzle mapping, rendered APP bits by section:

| Color | S1 | S2 | S3 | S4 |
|---|---:|---:|---:|---:|
| C | 10010 | 0 | 3849 | 9240 |
| M | 0 | 23373 | 0 | 0 |
| Y | 11543 | 0 | 11181 | 0 |

On the four-per-D fixed S1 inspection subset, actual APP counts are
D1=3080,D2=3080,D3=0,D4=0,D5=1545,D6=1520. Thus an APP-derived S1-only crop
really would omit magenta here, but would not have empty yellow data. The earlier
fixed S1 print generated 5688 bits per D independently and did not use this APP
selection. Do not explain earlier fixed-pattern yellow failure using a blank
APP payload, and do not equate fixed test lines to the original six image bars.

This result checks the APP data/content/decoder interpretation. It does not
validate the physical nozzle table, head drive or real ink output. The next
physical comparison should preserve the actual APP image's per-swath routing,
not silently replace it with a fixed region recipe.
