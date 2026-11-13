# Independent U5 boot scope session

This branch enables PRINTER_U5_BOOT_SCOPE=1. Start the entire machine from power
off with the existing HC32 firmware; its GPIO initialization leaves head CE,
VL and VP enables LOW before waiting for self-test. This BK boot mode does not
issue SPI, HC32 power/firing commands, APP jobs, or carriage motion. HC32 waiting
for its self-test is expected here. A live BK-only restart is not this test's
startup condition.

After normal BK startup banner delays (about one second), the task announces
starts_in_ms=3000. U5 runs after this additional three-second setup window.
It initializes motor GPIOs, checks/reclaims U5 pins, enables boost, sets 1/16
microsteps, LOW direction, wakes/enables U5, and generates STEP for 60 seconds
measured by the AON RTC. STEP HIGH uses 4us, LOW uses 1000us. RTOS yields every
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
