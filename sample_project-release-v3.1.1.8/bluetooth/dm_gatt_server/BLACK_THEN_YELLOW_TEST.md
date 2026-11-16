# One CMY black line followed by the two existing yellow lines

The user requested a black first line to test whether the symptom is confined
to yellow or to the first visible line. PRINTER_BLACK_PREFIX=1 builds one CMY
black overlay from the current upper yellow band's pixels (S1 rows29..43 with
the existing remap enabled). It renders the same shape in C/M/Y using the normal
per-domain X offsets. This printer has CMY channels, not a separate K channel;
black appearance depends on actual ink output/alignment. A dark line alone does
not prove all three components fired, so record whether it looks blue/purple.

All buffers and the black frame are prepared before motion. The prefix executes
one scan/return through the existing power/settle/stream helper, then the existing
yellow-only page runs without changed bytes or extra yellow copies. It is exactly
one added black pass, including one additional exposure of those S1 yellow
nozzles as explicitly requested, not an indefinite purge. No voltage, HC32 S/CE
calibration, motor GPIO or feed setting changes. Each scan still uses the existing
head ON/OFF sequence; this does not isolate color order within one uninterrupted
head-power session. The added scan/return and head cycle are experimental changes.

The black line is at the starting paper position. Ensure paper covers the head
area before triggering; it is not automatically pre-fed for this prefix. Existing
page feeds then place the yellow lines farther down the page. Tiny yellow dots
already present in the APP data are retained. Only the two main yellow bands are
expected to be clearly visible. There is no automatic repeat or end eject.

Startup: APP_BLACK_THEN_YELLOW_V1. Send the same original six-color image once
from the APP; do not edit the image to add black. BK inserts the one black line.
Use the current head/fault state without another wipe for the first observation.
Capture the whole page and BK/HC32 logs. First inspect black visibility/color and
the two yellows; no 20-minute idle is requested yet.

BLACK_PREFIX_PLAN fingerprints the frame, BLACK_PREFIX_BEGIN/END bracket the
first scan, and APP_YELLOW_SWATH/PAGE_PASS identify later yellow data. For the
identical task1059 input, black CRC84DA2B78 has 11521 mapped dots per C/M/Y.
Subsequent yellow swaths remain CRC1DA06B56 (22 dots) and CRC2F20CCB6 (22702 dots)
under the S1 nozzle-remap profile. New APP halftoning can vary these CRCs.

The actual controller test uses the real captured APP BIN and an independent
black-raster oracle, checks black-before-yellow order and exact unchanged yellow
bytes, and verifies OOM/link/scan/feed/head failure cleanup and lock behavior.
ASan/UBSan tests, existing S1/yellow/full-page/protocol regression and BK SDK
source-object compilation pass. Actual paper coverage, black color, ink output
and any recovery effect remain target-side observations, not software guarantees.
