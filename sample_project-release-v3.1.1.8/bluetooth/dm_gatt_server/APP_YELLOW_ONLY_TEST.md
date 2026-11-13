# Print only yellow from the actual APP page

PRINTER_APP_PAYLOAD_AUDIT=0, PRINTER_APP_YELLOW_ONLY=1. The normal full-page
controller bypasses the fixed S1/S2 inspection recipes. It validates and lays
out the actual APP BIN, masks every rendered column byte with 0x30 (D5/D6),
and emits each nonblank yellow swath once. C/M pixels are never transmitted.
It does not substitute a generated yellow line or increase per-nozzle dose.
Input without yellow is rejected before any motion. Startup requires the
normal HC32 link; failures keep the existing lock/no-blind-retry behavior.

Supply, HC32 sampling/continuous CE, GPIO17 MS3 and existing carriage pulse
engine remain unchanged. Normal page power/settle logic is used for each active
swath. This differs from the old fixed-inspection single power session because
this is actual page printing. Head is OFF for page feeds and blank strips do not
fire. The page layout retains six swaths and five 3170-pulse feeds, including
blank areas, rather than the old twelve 780-pulse diagnostic line spacings.
No automatic end eject is added. Physical feed distance remains a calibration
assumption, not encoder-confirmed motion.

## Target trial

Update only BK. Start the whole machine and confirm APP_YELLOW_ONLY_V1 and link
ready=1. Use the working-feed board and send the SAME six-line CMYYMC image from
the user's APP once. No wiping, cartridge reseating or long idle is needed.
Expect only the two original yellow bands and any tiny yellow pixels present
in the APP data, with blank spaces where C/M would be. It may feed before the
first yellow scan and after yellow ends to preserve original page position.
Wait for PAGE_DONE/PAGE_RESULT; do not judge the result by twelve fixed lines.
Save full BK/HC32 logs and the complete page, including blank areas.

APP_YELLOW_PLAN records source/resized yellow counts. APP_YELLOW_SWATH records
CRC/dots for all swaths; only nonblank ones generate APP_YELLOW_SCAN_BEGIN/END.
The legacy function name printer_execute_cyan_line is internal; the yellow
mode's emitted data and public log tags are yellow-only.

For identical captured task 1059 BIN (45246 bytes, CRC20C67068), source Y=34951,
resized Y=22724. Swath 2 has 22 small yellow artifact points, CRC1DA06B56;
swath 3 contains the two main yellow bands, 22702 points, CRC5FC50A3F.
Each active swath sends 1180 columns/118000 bytes in one head session. These
CRC/count expectations apply only to identical input; a new APP rendering may
change edge halftoning. Filtering retains all original yellow A/S/X values.
Successful transport is not proof of ink output.

## Verification

The actual controller is tested under ASan/UBSan with synthetic CMY pages and
the complete task1059 BIN: transmitted columns equal the original mapping with
only C/M bits cleared, no fixed recipe is called even when its old flags remain
enabled, blank-position feeds are retained, and empty Y fails before motion.
Allocation, link, scan, feed and head-OFF failures release memory and retain
proper locks. Legacy full-page, fixed-S1 and protocol tests pass. BK SDK source
objects compile. Hardware build/flash, actual paper advance and yellow output
remain target-side verification.
