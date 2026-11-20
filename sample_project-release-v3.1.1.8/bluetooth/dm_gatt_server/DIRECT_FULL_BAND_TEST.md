# Direct192-row nozzle-band comparison

This build maps image rows directly to physical nozzle rows0..191 on each
scan. Per color that is50 S1,50 S2,50 S3 and42 S4 nozzles; it is192 of200
physical nozzles, not all600. It does not use the interlace mask. An image point
is emitted exactly once, by its mapped nozzle. Empty source rows stay empty.
This is a diagnostic direct-band mode, not a claim to reproduce the sample
printer's multipass strategy.

Between scans the paper advances3170 pulses, the existing calibration for192
rows (~8.13mm). The last scan only emits remaining image rows; no trailing
full-band feed is added. A blank scan still traverses in bidirectional mode.
Physical image scale, source resizing, canvas width, carriage stroke/speed,
S sampling and tFire are unchanged. Scan/feed counts are lower than the24-row
mode; a corresponding image row can print on a different scan direction.
Changing nozzle allocation unavoidably changes the spatial/temporal load.

The band-feed helper now accepts up to3170 pulses, instead of500, and keeps
its existing800us start/400us cruise LOW pulse profile and64-step ramps. It
preserves carriage enable/microstep phase and the owned boost. It does not use
the old feed API that disables the carriage driver. Values above3170 are
rejected before movement.

Automatic boot cleaning remains disabled. Existing head-OFF startup homing,
pre-print wipe/loading and final cap parking are preserved. HC32 is unchanged.
Boot identification: APP_SERVICE_CMY_DIRECT_192_NO_BOOT_CLEAN_V40.
PAGE_BAND_PLAN must show rows=192; PAGE_FEED must show3170/3170.

## First device comparison

Prepare the head as for the S1 baseline and print the same CMY frame/diagonal
image once. Inspect continuity and density across all three colors. Send the
photo and both controllers' logs, especially PAGE_BAND_PLAN, PAGE_SWATH,
PAGE_FEED and HC32 LINE_DONE/RX_AUDIT. A host pass verifies mapping and protocol,
not physical ink delivery, band registration, paper travel or supply voltage.

Verification covers exact renderer dot totals, image boundaries, physical CMY
X alignment in both directions, final partial bands, sparse/blank scans,
consecutive pages, feed failures and failure locking. The actual U5 pulse
implementation is tested at3170 pulses and rejects3171 without motion; no U4
or boost reset is permitted. ARM SDK objects compile with the default profile.

## S1 restore point

BK tag backup/s1-24rows-before-fullband-20261120 points to358320d, with
24-row S1 printing and automatic cleaning OFF. For a separate baseline build
(save any uncommitted local work first):

```sh
git fetch origin --tags
git checkout -b test/s1-before-fullband backup/s1-24rows-before-fullband-20261120
```

Return to codex/printer-startup-load-prep to build this trial again. No HC32
rollback is required. Alternatively change only PRINTER_PAGE_BAND_ROWS back
to24 with PRINTER_PAGE_INTERLACE=0, leaving boot cleaning disabled.
