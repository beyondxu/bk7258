> Historical black/yellow diagnostic. Current firmware uses actual APP CMY.
> See SERVICE_CMY_PRINT_TEST.md for the active test.

# Resume black + yellow printing through the confirmed maintenance travel

Mechanical 8mm-per-side rightward travel and its faster600us cruise were reported
smooth. The user asked to resume printing rather than repeat the dry run.
PRINTER_SERVICE_TRAVEL_TEST=0, PRINTER_SERVICE_PRINT=1 restores the existing actual
APP black-prefix + yellow-page job and routes each active image scan through the
service-print helper. Maintenance spitting remains explicitly deferred.

The current revision extends each end by another approximately0.5mm (94 steps).
Margins are1590 steps (~8.5mm), total10660 steps (~57mm). To keep the image
position on the paper, establish the left origin0.5mm farther left than the
previous8mm mark. Pulse counts and millimeters remain approximate.

## Coordinates and one scan

Manually establish the same known LEFT8.5mm service start with power off before
startup. There is no automatic homing or encoder. In counted relative steps:

- 0..1590: leave left maintenance area, head OFF, no column SPI.
- 1590..9070: original7480-step print segment. Enable head here, retain the
  original2000ms settle,1180 columns/6steps and3000-to260us image ramp.
- At9070: acknowledge head OFF before any more movement.
- 9070..10660: enter right maintenance area, head OFF, no column SPI.
- 10660..0: return left, head OFF, no column SPI, then disable boost/motor.

Service segments/return use the mechanically tested600us cruise with3000us
ramp endpoints. New non-paper travel is never ink-enabled. The legacy print
window and its200-step blank lead/tail are unchanged; actual paper boundaries
still require physical alignment. One successful pass has10660 forward and
10660 reverse pulses and1180 transmitted columns. On handshake/SPI/partial-motion
failure stop and request head OFF, with no blind return. The page failure latch
then blocks further automatic scans/feeds until a deliberate restart/origin reset.

U5 feeds occur while parked at the left service position with head OFF. Head
ON/OFF sessions, voltage and HC32 waveform configuration are unchanged. The
extended motion can operate the mechanical wiping/pad parts but does not prove
correct contact, wiping direction, sealing or restored ink output. Reverse
service travel should also be observed for binding; pulse counts are not feedback.

## One initial print

Update BK only and start the whole machine normally with paper covering the
print area and the carriage at the known left service mark. No additional manual
wipe or head swap for this observation. Startup should say APP_SERVICE_BLACK_YELLOW_V1.
Send the same original six-color APP image once. BK inserts one CMY-overlay
black line first, then retains the current S1-remapped upper yellow and the
unchanged lower yellow. Minor original APP yellow dots remain. There are no
added repeat fires or maintenance spitting. Black uses CMY, not a separate K
channel, so record whether the first line is dark/blue/purple as well as its
presence. Capture the complete paper and BK/HC32 logs through PAGE_RESULT.
Do not start a20-minute idle study before reviewing this print baseline.

PRINT_SERVICE_MOVE marks non-firing LEAVE_LEFT, TO_RIGHT and RETURN_LEFT.
PRINT_WINDOW_BEGIN marks the only region allowed to send image columns.
PRINT_SERVICE_END reports code, column count, forward/reverse pulses and net
relative pulse position. Net zero means counted return, not measured homing.

## Verification

An ASan/UBSan harness runs the ACTUAL original image-stream function, ACTUAL
service pulse engine and new service-print helper. It checks every SPI column
against the exact original data and absolute1590+200+6*n position, only forward
image firing, head OFF on all service/return edges,10660 forward/reverse counts
and return to zero. Head OFF/ON errors, SPI failure, right extension and partial
return errors stop without blind movement. Existing black/yellow exact-frame,
service-travel, legacy motion and receive protocol regressions pass. BK SDK
startup, worker, protocol and motor objects compile. Full firmware linking,
physical paper alignment, maintenance contact and actual ink output remain
hardware verification; no claim of fixing the original idle ink failure yet.
