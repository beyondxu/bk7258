# Restore the pre-S1 printer baseline for paper-feed comparison

All existing files in this project are restored exactly to commit 2de4c9e.
The only addition relative to that project snapshot is this document. This is
a forward rollback commit on top of the diagnostic history, so ordinary
fast-forward merging from those diagnostic branches works without resetting
or discarding local work. Previous versions remain in history.

Reason: the user's 11:26 task 1031 completed the fixed-S2 12-domain inspection
and produced visibly separated lines. Paper-feed nonresponse was first reported
after moving to S1. This selects a recent pre-report baseline for comparison;
it does not prove S1 caused the feed issue or that rollback will repair it.
The later nFAULT/microstep/mapping observations remain unresolved.

Restored behavior: normal BLE/APP startup and HC32 link self-test, no automatic
boot motor run, no feed-only APP interception, fixed S2 12-line inspection,
original motor initialization/feed path. Keep the existing HC32 firmware and
supply unchanged. MS3 remains GPIO17 as in the 2de4c9e baseline and schematic.
Preserve the link-block diagnostics and corrected data-fault reporting already
present in that baseline.

Update/flash BK, start the whole machine normally, and check startup link ready=1.
A BOOT_LINK_UNAVAILABLE block means the motor test never started. Trigger one
APP job, with paper loaded as for the prior test. Observe actual paper movement
after the first scan; do not infer movement from software pulse counts alone.
Do not wipe/remove the head or add a long idle interval for this first comparison.
If paper stays stationary, stop the trial, save the complete log and avoid
repeated printing in the same position.

Expected startup: APP_FIXED_S2_DOMAIN_CHECK_V1, no U5_BOOT_BEGIN.
Expected inspection: D1,D2,D3,D4,D5,D6,D6,D5,D4,D3,D2,D1;
14160 columns, 1416000 bytes, CRC A13F47F7, A=004A2912 S=02 D=3F,
5688 set bits per D. Intermediate feed: 780 pulses; final feed: 1560.
Transport completion is not actual motion or ink feedback.

Verification: project production/test files match 2de4c9e exactly; actual
controller, receive/fault protocol and feed tests plus BK SDK source-object
compilation are rerun. Physical feed behavior requires this target-side trial.
