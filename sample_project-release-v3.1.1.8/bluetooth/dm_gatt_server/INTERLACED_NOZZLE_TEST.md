# S1–S4 interlaced nozzle trial

The baseline used the same first24 physical nozzle rows on every normal-page
scan, all in S1. This trial keeps24-row paper advances and uses a192-row physical
nozzle window, covering S1/S2/S3/S4. Maintenance retains its existing600-nozzle
mapping and doses. HC32 source and firmware are unchanged (79ba462).

Each source pixel is allocated to exactly one scan/nozzle pair. It is never
printed eight times. If a source pixel has Y coordinate y and a selected nozzle
block j (24 rows per block), it prints on scan floor(y/24)-j using nozzle
24*j+y%24. Paper origin plus nozzle row remains y. CMY and reverse scans use the
same allocation before applying existing physical X compensation.

At the leading edge, only blocks that do not require negative paper travel are
eligible. The first24 image rows necessarily still use S1. Eligibility expands
to eight blocks by row168; no leading margin, extra scan or feed is added.
Normal pages use192 of200 nozzles per color, not all600. End rows outside the
image remain zero. This is load spreading, not bad-nozzle compensation or proof
of the cause of drying. Leading-edge scans can contain more dots than baseline
scans because they also print some future image rows; page totals are identical.

Unchanged: STEP timing, stroke distances, paper calibration, page resizing,
image transforms, S pulse width, tFire, power sequence and boot cleaning.
Rendering completes before each line starts; the full118000-byte HC32 line
buffer remains unchanged. The trial is enabled by PRINTER_PAGE_INTERLACE=1;
setting it to0 restores the previous normal-page renderer.

## Verification and device comparison

Host tests decode actual C renderer bytes back into source pixels, checking
exactly-once coverage, first/last bands, short pages, sparse/dense content,
S1–S4 usage and forward/reverse X compensation. Controller tests cover unchanged
fractional feeds, failure locking and two consecutive pages in both modes.
ARM SDK object compilation checks integration. These checks do not establish
actual paper direction, registration, ink ejection or temperature on hardware.

For the first comparison, print the same CMY rectangle/diagonal test once after
the usual preparation. Compare vertical dashes and band alignment to baseline;
collect PAGE_INTERLACE, PAGE_SWATH and HC32 RX_AUDIT logs. Do not change tFire
at the same time. A later idle comparison can test the10-minute symptom.

## Restore point

Both repositories have annotated tag backup/nozzle-s1-baseline-20261120:
BK bbcb07c and HC32 79ba462. Preserve local work before switching branches.
For a separate baseline build, in each repository:

```sh
git fetch origin --tags
git switch -c test/nozzle-s1-baseline backup/nozzle-s1-baseline-20261120
```

This creates a separate branch without resetting the trial branch. To return,
switch back to codex/printer-startup-load-prep (BK) or
codex/printer-cached-line-timer (HC32). Older Git may use checkout -b instead
of switch -c. No HC32 rebuild is needed for this BK-only trial if79ba462 is
already flashed.
