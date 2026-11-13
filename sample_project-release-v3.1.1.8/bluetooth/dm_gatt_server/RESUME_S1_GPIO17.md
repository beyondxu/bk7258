# Resume the latest S1 ink inspection with schematic GPIO17

The user reports the paper motor operates on another board and wants to leave
the temporary motor diagnosis. Restore the project snapshot c9e2041 exactly;
this document is the only addition relative to that snapshot. This is a new
forward commit so the user can merge normally without discarding history.

U5 MS3 is GPIO17 again. APP_FIXED_S1_COMPARE_V1 is the active inspection;
there is no boot motor loop, feed-only interception, extra U5 reconfiguration,
or temporary GPIO16 assignment. Preserve original feed pulses/timing, link
startup/first-failure diagnostics and corrected data-error reporting.
HC32 sampling, continuous CE and voltage remain as currently installed.

Flash BK, start the whole machine normally and confirm PRINTER_LINK_STATE ready=1.
On the board with working feed, send one APP job without wiping/removing the
head. A blocked startup is not an ink test. Save complete BK/HC32 logs and the
full page, including spaces for missing lines. Check actual paper movement
after the first scan. No artificial ten-minute wait is needed for this first
resumed S1 trial.

Order: D1,D2,D3,D4,D5,D6,D6,D5,D4,D3,D2,D1; expected visible colors
C,C,M,M,Y,Y,Y,Y,M,M,C,C. One job has 14160 columns, 1416000 bytes,
CRC FE431638, A=004A2912, S=01, D=3F, each D has 5688 set bits.
Each selected nozzle gets 1422 commands in total, without priming or x2.
This trial compares a different physical nozzle area to the former S2 recipe;
it does not by itself identify why a color is absent.

The S1 exact-byte controller, page-feed and receive/fault regression tests
and BK SDK object compilation pass. Actual motion/ink remain target-side
verification. The faulty board's component-level cause remains unresolved.
