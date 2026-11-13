# Diagnose APP data-error reports caused by an execution block

Observed task 1028: received=expected=37329; received and expected CRC32
A3E603F2 matched (AUDIT_RX PASS). PAGE_RESULT then returned -7.
For the active fixed-S2 inspection controller, -7 means the startup link was
unavailable or a previous execution failure remains locked. The excerpt does
not identify which condition occurred or the original failure that set the lock.

Previously page/line completion set status Bit3 (command/data corruption) for
all failures. Keep the failed finish response, but reserve Bit3 for receive or
saved-buffer integrity failures and data/render validation errors. A link block
has no dedicated documented status bit: report finish=FAIL, status_fault=0.
The APP may still show a generic failure based on its finish-result handling.

## New evidence

- PRINTER_LINK_STATE ready=0/1 records the final startup self-test outcome.
- DOMAIN_CHECK_BLOCKED reason=BOOT_LINK_UNAVAILABLE or PREVIOUS_FAILURE_LOCK
  records both guard flags, the first failure code/stage/line, and completed runs.
- DOMAIN_CHECK_LOCK records the first failure as it happens. Stages distinguish
  frame/job validation, initial head OFF, head ON, scan, feed and final head OFF.
  Line numbers are one-based; zero means execution has not reached a line.
- PAGE_FAULT records execution code, input integrity, status byte and failed finish.

No automatic unlock/retry, extra firing, or change to SPI, handshakes, motion,
head timing, inspection order, or frame bytes. This change diagnoses the block;
it does not claim to repair the underlying link/first execution failure.
HC32 firmware need not change.

## Capture

After updating BK firmware, start both MCUs normally and capture the complete BK
and HC32 startup logs. Trigger one APP job. Save the first failure and the above
new lines. Do not repeatedly reset or re-send before saving the first error.
If the link is unavailable, the job intentionally performs no motion or firing.

## Validation

Actual protocol completion tests cover failed finishes without false Bit3 for
link, power, transfer, feed, deadline and allocation failures; real data/CRC
failures retain Bit3. Actual six-color and twelve-domain controllers verify
blocked guards perform no work, first-error provenance survives subsequent
blocked calls, normal golden frames/order/CRCs and failure cleanup are preserved.
Host tests use ASan/UBSan; controller harnesses also compile for ARM. BK SDK
object compilation checks protocol, worker/controller and startup units.
Hardware reproduction and full firmware linking/flashing remain to be done.
