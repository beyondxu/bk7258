# Temporary counted continuous boot carriage test

Branch: codex/printer-boot-continuous-carriage-test, based on344ba4c.
Normal printing remains on codex/printer-startup-load-prep, without these
changes. HC32 firmware is unchanged and is needed for self-test/head-OFF ACK.

## Current V43 mode

PRINTER_BOOT_CARRIAGE_LOOP=1; PRINTER_BOOT_LOOP_IGNORE_OC3=1.
After the link self-test, the carriage continuously runs10473 pulses RIGHT,
then10473 LEFT. This uses the configured print-stroke length, shorter than the
11221-step cleaning/service stroke. Start from the usual wiper/cap-side origin.
It is counted motion; no physical endpoint or lost-position detection remains.

GPIO24 input level is not polled or used to stop/reverse movement. While the
loop speed profile is active, timer claim does not require OC3 initialization,
its motion callback is detached, and both IRQ and level stop paths are bypassed.
GPIO24 may still be configured by the shared motor GPIO initializer; it has no
control over this temporary loop. Bypass is limited to the boot-loop profile;
normal printing's OC3 handling is unchanged. Setting IGNORE_OC3=0 restores the
previous optical seek/fixed return mode for this temporary branch.

TIMER4 produces STEP75us (HIGH27us, LOW48us), ~9.3% faster than82us.
Start period400us and64-step ramps are retained. The task yields1ms per round;
motor boost and microstep phase are retained between legs. Pulse counts,
5-second leg timeout, timer ownership and late-IRQ guards remain active. Any
failure stops the timer/motor without a blind return or automatic retry.

Head power stays OFF. No image is armed, no column-start SYNC or paper feed
is executed, and no maintenance cleaning runs. BLE/print workers are not
started while the loop runs; a loop error leaves printing blocked. Power OFF
stops the test. This is a temporary motor test, not unattended endurance mode.

Startup identifies itself as BOOT_MODE / TEMP_COUNTED_NO_OC3_V43. Look for
ignore_GPIO24=1 counted_leg_steps=10473, then per-round RIGHT/LEFT counts.
BEGIN/RESULT/STOP logs include timer errors. GPIO-active fields are0 placeholders
in bypass mode, not measured levels. Incompatible build profiles fail compilation.

Tests use the actual timer engine for three complete fixed round trips then
inject a late-IRQ fault. They verify75us spacing, exact10473-step legs, zero
GPIO24 reads/ready checks, detached callbacks, zero ink/paper and fault stop.
Cases include initially LOW/stuck input, uninitialized sensor, and a falling edge
mid-stroke. The alternative optical mode and original motion regression are
also checked. Host tests do not establish physical position or real timing.

Restore normal printing by switching BK to codex/printer-startup-load-prep,
rebuilding and flashing. HC32 requires no changes. The temporary speed/bypass
profiles are inactive when PRINTER_BOOT_CARRIAGE_LOOP=0.
