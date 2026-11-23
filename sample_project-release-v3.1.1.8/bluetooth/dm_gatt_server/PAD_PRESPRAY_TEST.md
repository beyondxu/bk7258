# RTC-based sponge-pad prespray

Normal branch: codex/printer-startup-load-prep. Image printing stays24-row S1,
with original size, feed, carriage speed/stroke and HC32 firmware unchanged.
This adds ordinary-drive spit before each validated full-page job and at boot.
It does not read TSR, preheat, shorten CE, raise VP or select a2.2us drive.
The old twelve-row long boot clean remains OFF and cannot be enabled together
with this policy. There is no background idle spit or new low-power resume hook;
a print after wake/idle runs the same pre-page policy.

## Dose and clock

Time since the latest successfully completed ink-request scan/flash is tracked
with64-bit BK AON RTC microseconds, in serialized worker/boot context:

| Idle interval | Requests per physical nozzle | Total requests | Cache rows |
| --- | ---: | ---: | ---: |
| Under3minutes |5|3000|1|
|3minutes to under15minutes |50|30000|2|
|15minutes or longer |200|120000|5|
| Unknown after boot or RTC rollback |200|120000|5|

The clock is updated after successful nonblank image scans, completed prespray
batches, paper maintenance scans and manual waste-clean batches. Blank carriage
scans do not update it. It records requested firing, not measured ink drops.
No last-ink timestamp is persisted across power loss; unknown uses the long dose.
The selected dose is captured before pad positioning and is not reduced midway
when a successful batch updates the clock. Failed/partial batches are not retried.

## Placement and sequence

Use the established wiper-side starting position and connected active-LOW
GPIO24 reference. Move head-OFF to the sponge-side optical reference, retract
374 steps into the previously user-confirmed pad coverage (PRINT_RIGHT), stop
and release TIMER4, and only then allow stationary cached firing. No paper feed
or image data is sent during positioning. A missing/early/stuck optical signal
or motion fault rejects firing. Cached-row CRC, READY, finite count and timeout
checks remain active. Any fault stops and blocks the image job; there is no
blind return or automatic repeat after an uncertain/partial firing operation.

After all batches, command head OFF and return/park at the wiper. At boot this
replaces the previous head-OFF-only reference round trip, before BLE starts.
For image jobs it runs in the existing worker after parsing/allocations and
before the unchanged wipe and six-leg10.8mm paper-loading preparation. That
preparation then runs once. No extra wipe is added by prespray itself.

## Timing and limits

Each round covers all600 physical nozzles (S1..S4, all CMY), not just the image's
first24 rows.24 requests are distributed per transport column;25 columns cover
600 nozzles. This is an upper bound of24 requested bits over the whole column,
not a command to fire600 simultaneously. Within a column the existing HC32
25-address/S1..S4 sequence remains unchanged.

A nozzle repeats after25*673us=16.825ms (~59.4Hz), below the18kHz limit cited by
the user. Every existing column retains its25+1 zero-data stop, Ax/Dx reset and
minimum60us completed-column idle guard. There is no faster sampling path.
Up to40 rounds fit one1180-column row; doses split as5,40+10,or40*5.
Playback is~0.8/1.6/4.0seconds, plus cache transfer and motion/parking time.
No temperature or recovery claim follows from these limits.

## Verification and first hardware test

Host checks independently compare nozzle routes against the600-row CSV and
verify exact dose, peak requests, repeat gaps and bad-input rejection. Tests
exercise3/15minute boundaries, zero timestamp,64-bit values and RTC rollback;
job failures and allocation cleanup; actual boot glue; page-before-preparation
ordering and consecutive jobs; actual optical positioning and stationary SPI
playback with zero U4/U5 pulses. SDK ARM object compilation checks integration.
Hardware must still establish pad coverage, actual drive timing, power and ink.

First test: after a long idle, power ON and inspect the BOOT prespray log, then
print the same CMY frame once. BOOT normally selects200; a prompt PAGE selects5.
Capture PRESPRAY_BEGIN/BATCH/DONE (or STOP) and the print result/photo. Logs
identify APP_SERVICE_CMY_PAD_PRESPRAY_V44. Recovery remains to be verified.

## Baseline

BK backup/before-auto-prespray-20261123 points to344ba4c (S1, no auto spit).
Preserve local edits, fetch tags and create a separate branch for a baseline
build if needed. PRINTER_PRESPRAY=0 restores the original boot and image flow.
The independent temporary motor-loop branch is not merged into this branch;
GPIO24 is fully active for normal pad positioning. HC32 stays79ba462.
