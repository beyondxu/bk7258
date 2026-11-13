# Bounded U5-only diagnosis after S1 paper did not move

Evidence: task 1040 completed all 12 software scans and feeds with expected S1
CRC FE431638, but the user observed U5 stationary and silent. The existing
standalone motor firmware still advances paper on the same machine. This makes
an application control/configuration issue a priority; it does not identify the
root cause. The S1 change did not modify the motor driver or feed helper.

## Temporary APP behavior

PRINTER_U5_FEED_ONLY=1 replaces the inspection print after normal native-F2 input
validation. An APP job requires a ready startup link and an acknowledged head
OFF command, then executes one 780-pulse U5 feed. There are no print frames,
carriage scans or nozzle firing. U5 and boost are turned off when the feed ends.
A failure remains locked; there is no automatic retry. HC32 stays unchanged.
APP success means software feed completed, not verified physical movement.

The startup banner is APP_U5_FEED_ONLY_V1. Capture U5_ONLY_BEGIN, U5_GPIO,
U5_STEP_LATCH, U5_ONLY_END and PAGE_RESULT. One APP request runs one short feed;
there is no need to wait ten minutes or wipe/remove the head. Observe whether
the motor/rollers and paper move. End this trial after one request.

## Candidate correction and evidence

Before a page feed, park U4 as before and reclaim only the eight U5 output pins
using gpio_dev_unmap and checked bk_gpio_set_config. Hold EN disabled, STEP low,
reset and sleep low, then apply existing microsteps, wake/reset/enable delays,
direction and pulse timing. This addresses possible pinmux/configuration loss;
it is not yet proven to be the cause. Configuration errors return before pulses.
The keep-power feed path does not call the global GPIO initializer or toggle
the shared boost. The head-OFF standalone diagnostic owns its boost as usual.

Read-only U5_GPIO snapshots record STEP18, EN44, RESET53, SLEEP54, DIR55,
BOOST13 registers plus nFAULT45 before reclaim, ready and after pulses. The
first STEP-high register is saved during the pulse and printed only after all
pulses, so UART logging cannot stretch a live STEP. Register output latches and
software pulse counts are not measurements at the driver pins or motor shaft.
nFAULT is logged only, not used to introduce an unverified abort criterion.
On BK7258 register bit1 is the output latch, bit3 is output disable (active low
enable), and bit6 enables the secondary function. Input bit0 can affect raw hex.

Set PRINTER_U5_FEED_ONLY=0 to restore the S1 inspection after diagnosis. Do not
interpret this feed-only trial as an ink/color result.

## Verification

The actual feed implementation is tested for wake/reset/enable before edges,
U5-only pin configuration, exact pulse counts, preserved boost in keep-power
mode and no pulses on unmap/config failure. The actual APP controller branch
is exercised with ASan/UBSan for one feed with head OFF and no carriage/firing,
plus startup blocking and failure locking. BK SDK object compilation checks
motor, worker, startup and protocol. Full firmware linking/flashing and physical
motion verification remain target-side work.
