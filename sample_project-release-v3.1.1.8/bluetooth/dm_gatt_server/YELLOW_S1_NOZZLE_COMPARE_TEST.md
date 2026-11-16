# Compare a different S1 nozzle subset while keeping the S3 yellow control

The user reports the upper yellow line degrades after 10 minutes, often disappears
after 20 minutes, and immediate reprinting does not recover it. Wiping can restore
two lines. No diagnosis of drying, bubbles, permanent damage or S1 circuitry is
established. This firmware changes the selected upper nozzles to distinguish a
subset-dependent symptom from a wider region/drive issue.

PRINTER_YELLOW_S1_REMAP=1 adds a preflight raster relocation to the existing
actual-APP yellow-only page path. In swath 3 only, yellow local rows 9..23 move
to rows 29..43. Both sets are S1 according to the candidate nozzle table and are
disjoint physical routes; a 20-row shift preserves the D5/D6 pairing and hence
the per-pixel X compensation. Each original pixel is moved once, not duplicated.
Upper line position shifts by about 0.847mm toward the lower line. S3 and all
other rows/swaths are byte-identical. C/M are still masked out before transmission.

Original APP counts and per-nozzle exposure pattern are retained one-to-one on
the new nozzles. Voltage, HC32 timing, power/settle sequence, feeds, motor GPIO17,
scan order, lower line and the minor yellow artifact pixels remain unchanged.
This is not a hardware rewiring or a global S1 timing change.

Before mutation, validate the geometry/route pairing and check that all yellow
S1 pixels in swath 3 are inside the expected old band. A different layout or an
occupied destination is rejected before any motion. An empty upper band is also
rejected. This deliberately supports the already-audited six-color test image,
not arbitrary images. The whole incoming BIN is still validated before printing.

## Initial user action only

Update/flash BK; leave HC32 unchanged. Start the whole machine normally, confirm
APP_YELLOW_S1_REMAP_V1 and link ready=1. If the upper line is currently absent,
perform the user's normal single wipe/reinstall, then print the same six-color
image once. Save the full page and BK/HC32 log. First confirm two yellow lines;
do not immediately start a 20-minute wait or keep printing if this baseline fails.
The next timing instruction is given after reviewing this baseline.

YELLOW_S1_REMAP logs moved count and old/new rows. APP_YELLOW_SWATH logs emitted
CRCs. For identical task1059 input: swath2 CRC1DA06B56/22 bits is unchanged;
swath3 changes from CRC5FC50A3F to CRC2F20CCB6, retaining 22702 bits. Different
APP edge halftoning can change CRCs. Wire success is not ink feedback.

## Verification

The actual controller is tested with the captured APP BIN against an independent
raster-shift oracle. Every S2/S3/S4 byte and every other swath is unchanged; bit
counts are conserved. OOM/link/feed/scan/head failures retain cleanup/locks.
ASan/UBSan direct tests verify exact relocation, disjoint physical routes and
no mutation on unexpected input/occupied destination/config rejection. Legacy
yellow-only regression and BK SDK startup/motor/protocol/worker object compilation
pass. Target-side paper/idle behavior remains to be tested.
