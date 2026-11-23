# Temporary continuous boot carriage test

Branch: codex/printer-boot-continuous-carriage-test, based on344ba4c.
Normal printing remains on codex/printer-startup-load-prep and is not changed
by this branch. HC32 firmware is unchanged and must be connected for the boot
self-test and head-OFF handshake.

After the link self-test succeeds, the carriage continuously shuttles between
the existing wiper/cap starting anchor and sponge-side GPIO24 optical endpoint.
It uses TIMER4, the existing bounded boot seek and11221-pulse LEFT return.
Start at the same wiper/cap position as the verified printing firmware.
Cruise STEP period is75us rather than82us (~9.3% faster); HIGH stays27us,
LOW48us. Start period400us and64-step ramps remain. There is no added long
turnaround dwell; the task yields1ms after each round trip. The motor power
session and microstep phase are retained between trips.

Head power stays OFF; no image is armed, no column start SYNC is sent, no paper
feed API is called, and no maintenance cleaning runs. BLE/print workers are
not started during the continuous loop. This test is stopped by powering OFF.
On an error the loop stops, cancels the timer, releases motor power and blocks
printing with no automatic retry. A missing optical endpoint stops at the seek
cap without a blind fixed return; an initially active GPIO24 blocks movement.
The wiper side still uses the previously verified fixed return count, not a
second sensor. This is a temporary motor test, not an unattended endurance mode.

Logs: BOOT_CARRIAGE_LOOP, BOOT_CARRIAGE_LOOP_ROUND and
BOOT_CARRIAGE_LOOP_STOP. Inspect actual movement/noise on the first trips.

Host tests exercise the real timer engine over three complete optical-reference
round trips, checking75us pulse spacing, zero HC fires and paper pulses, then
inject a timing fault to end the otherwise infinite loop. Additional cases
cover failed link, initially LOW optical input, and missing optical input.
ARM SDK object checks cover actual startup, worker/motion and motor sources;
hardware timing and physical position are not established by host checks.

Restore printing by switching BK back to codex/printer-startup-load-prep,
rebuilding and flashing. No HC32 changes are needed. Setting
PRINTER_BOOT_CARRIAGE_LOOP=0 also disables this mode; the separate75us speed
profile is only active inside the temporary loop, not normal print movement.
