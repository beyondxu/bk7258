# First service-station travel trial: approximately 10mm each side, no ink

The user observed physical wiper/absorber-like parts outside the current travel
and confirmed the current carriage never reaches them. They estimate 7480 steps
as about 40mm and explicitly requested trying 10mm each side first. Component
functions and whether any parking pad actually seals the head are not proven
from the photo. Automatic maintenance may be missing; causality remains unproven.
Public maintenance-document searches were blocked by environment network policy.

## Geometry and motion

1870 steps = 7480 * 10/40; endpoint-to-endpoint span = 11220 steps (~60mm).
The OLD known scan start is relative coordinate zero; this is not auto-homing.
Start from that same physical position as before. Do NOT manually shift the
start 10mm, which would apply the offset twice. One APP job executes:

1. Left1870 to relative -1870 (left maintenance zone).
2. Right11220 to relative +9350 (old right endpoint7480 plus1870).
3. Left11220 to relative -1870.
4. Right1870 to relative 0, restoring the old start.

Total26180 pulses; a slow 1000us cruise low time and 3000us ramp ends, 4us HIGH,
RTOS yields and pauses give roughly30 seconds. This first trial is slower than
normal printing to inspect travel/contact. Approximate40mm calibration does
not establish exact10mm extension, physical endpoints or absence of lost steps.
There is no limit-switch/encoder stop. Observe the motion; stop power if it
approaches a hard stop, binds, or makes an abnormal sound. A bad origin or lost
steps requires physically restoring the old origin before any later test.

## No printing and no automatic repetition

PRINTER_SERVICE_TRAVEL_TEST=1 takes priority over black/yellow printing in the
worker. Receive length/CRC remain validated. The head-OFF command must be
acknowledged before boost/motion. There are no head-ON commands, no firing
columns, no paper feed, and no startup auto-motion. All travel, including the
paper area, is non-firing in this trial. The mechanical wiper can be observed
without coupling a first stroke test to printing. HC32 firmware is unchanged.

U4 and boost turn OFF after success or a reported partial-leg failure. No blind
full return is attempted after a partial/error result. A one-shot latch blocks
another APP-triggered cycle until restart. Hardware motor protection remains;
software counters do not detect an actual collision or stalled carriage.

## One target action

Update/flash BK, start the whole machine normally with the carriage at its old
known scan start, confirm CARRIAGE_SERVICE_DRYRUN_V1 and link ready=1. Send the
usual APP image once. No paper or wiping is needed. Watch whether it reaches
both maintenance parts without a hard stop and returns to its old start. Save
SERVICE_BEGIN, SERVICE_LEG, SERVICE_POSITION, SERVICE_END and the HC head logs.
First confirm motion/contact only; actual print-window integration follows
that observation. The old black/yellow feature remains in source but inactive
under this dry-run override. Do not interpret a blank page as print failure.

## Verification

Actual service motor loop and controller run under ASan/UBSan with a fake clock.
Tests verify four exact legs, endpoints -1870/+9350, net-zero pulses, slow timing,
head OFF before any steps, no firing/feed path, bounded arguments, link/OFF
failure blocking, no blind return on partial/error results and the one-shot lock.
Existing black/yellow/full-page/feed tests pass. BK SDK source objects compile.
Physical distance, actual contact, safe endpoints and recovery of ink output
remain target-side verification.
