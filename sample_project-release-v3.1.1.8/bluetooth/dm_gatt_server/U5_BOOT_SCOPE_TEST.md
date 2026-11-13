# Independent U5 boot scope session

This branch enables PRINTER_U5_BOOT_SCOPE=1. Start the entire machine from power
off with the existing HC32 firmware; its GPIO initialization leaves head CE,
VL and VP enables LOW before waiting for self-test. This BK boot mode does not
issue SPI, HC32 power/firing commands, APP jobs, or carriage motion. HC32 waiting
for its self-test is expected here. A live BK-only restart is not this test's
startup condition.

After normal BK startup banner delays (about one second), the task announces
starts_in_ms=3000. U5 runs after this additional three-second setup window.
It initializes motor GPIOs, checks/reclaims U5 pins, enables boost, selects half-step
mode, LOW direction, wakes/enables U5, and generates STEP for 60 seconds
measured by the AON RTC. STEP HIGH now uses 10us, LOW uses 3000us, matching the pulse arguments in
wifi/provisioning/with_ble/ap run_dual_motor_test. MS1/MS2/MS3 are set to
1/0/0 through MODE_HALF_STEP, matching that function rather than the earlier
all-HIGH mode. RTOS yields every
32 pulses and UART status every five seconds add gaps with STEP LOW. There is
no UART output during a STEP HIGH. Scope readings must establish real levels.

When time expires, STEP is LOW, U5 is disabled/reset/asleep, and boost is OFF.
The app task stays idle and does not proceed into SPI self-test or BLE/APP
startup. There is no automatic repeat. Power cycling starts a fresh session.
Early GPIO configuration failure disables the drive/boost and generates no
pulses. The existing power-key task remains available.

Capture U5_BOOT_BEGIN, U5_GPIO BOOT_READY, U5_BOOT_RUN, U5_BOOT_END. Probe U5 STEP
(GPIO18), EN (GPIO44), RESET (GPIO53), SLEEP (GPIO54) with common board ground.
Expected while running: STEP pulses, EN LOW, RESET/SLEEP HIGH. Check driver-side
VM if these levels are correct but the motor remains silent. Raw register
snapshots and pulse counts are software evidence, not physical feedback.

The bench timing harness executes the actual 60-second loop with a fake RTC
(no real-minute test wait), checks stop/cleanup and configuration failure, and
checks boot routing precedes self-test and cannot fall through to APP work.
The actual page-feed regression and BK SDK compilation of startup, motor,
protocol and worker pass. Full firmware build and physical scope test remain
for the target. This mode changes motor timing for scope observation only;
it does not establish or repair the original stopped-paper root cause.

## Comparison limits

The reference wifi motor header defines U5 MS3 as GPIO16; the active printer
header defines GPIO17. This branch retains GPIO17 because previous printer
versions reportedly fed correctly; source disagreement alone does not establish
the PCB connection or root cause. U5_MS_GPIO reports registers 16 and 17, plus
MS1/2 and the selected pin number, for further checking. Do not treat all-HIGH
as a verified 1/16 setting: the project enum assigns MODE_1_16_STEP=3, not 7.
The board's actual microstep truth table must be checked before changing normal
page motion and its feed-distance calibration.

Only the boot scope profile changes in this branch. Normal page-feed timing,
GPIO mapping, 60-second limit, no-APP/no-HC gate, no-firing behavior and startup
stop/cleanup remain unchanged. MODE_HALF_STEP has more shaft movement per pulse
than a finer subdivision; remove the test page if travel is inconvenient. The
reference test also exercises both directions; this boot trial keeps direction
LOW to avoid an unrequested reverse feed. Neither direction setting proves the
actual mechanical forward direction.
