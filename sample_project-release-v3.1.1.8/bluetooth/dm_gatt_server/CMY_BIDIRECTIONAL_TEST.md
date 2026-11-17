# Fast alternating CMY band test

The previous captured24-row job required about19.1s per nonempty band, including
an empty11408-step return and2000ms head settle; each396/397-pulse feed took
about1s. The user requested actual faster printing in both directions, matching
the sample printer's observed workflow rather than more slow empty returns.

## Active behavior

APP_SERVICE_CMY_BIDI_V3 starts from the same manually established LEFT origin.
Actual24-row CMY bands alternate RIGHT, LEFT after each nonempty image scan.
Advance paper after a band before printing the next nonempty band in the opposite
direction; blank bands retain the original fractional396/397-step feeds and do
not move the carriage or toggle direction. There are no duplicate image pixels.
The first scan starts LEFT and stops RIGHT; the next starts RIGHT and stops LEFT.
Only if an odd number of image scans completed, park LEFT once with head OFF at
page completion. No automatic empty return after each band.

Keep1964-step non-paper margins (~10.5mm each),7480-step image travel,11408-step
full span and the manual origin. Only enable the head within the original image
segment. Head OFF during both maintenance margins, paper feed and final parking.
No maintenance firing, extra black prefix or startup firing is added.

The dedicated bidirectional step engine uses HIGH4us, LOW1000-to80us ramps
(250 steps at each leg end). Previous image LOW260us and service LOW600us remain
in legacy/inactive helpers. Remove the2000ms diagnostic head settle; retain
shared boost75ms wake/settle and existing HC32 power handshake. SPI/READY waits
remain synchronous and abort on failure; HC32 pulse width, voltage, per-column
reset and required refill delay are unchanged. Nothing claims80us equals the
actual step period while waiting on SPI, or guarantees the sample's0.67s scan.
Host simulation with350us per-column handshake predicts about2.17s per full
image traversal, compared with the prior observed19s scan/return cycle. Target
motor torque, lost steps and actual timing must be observed.

## Reverse registration

Mirror source X and invert the mapper's domain-delay direction together; keep
nozzle rows/color masks unchanged. Reverse scan sends chronological columns
from the right boundary. Forward fires before edges200+6*n. Reverse fires before
edges206+6*n, so7480-206-6*n = 200+6*(1179-n). Both directions address exactly
the same nominal physical1180-column grid; no one-column boundary offset.
This does not calibrate mechanical backlash, jet flight-time or real color
registration. Previously observed vertical seams retain the3170-step/192-row
feed calibration and may still need measurement.

## Logging and failure behavior

BIDI_SCAN reports direction, code, columns, directional pulse totals, full elapsed
microseconds, aggregate/max per-column communication wait, and counted endpoint.
BIDI_PARK is head-OFF final positioning only. Any scan/handshake error invalidates
counted position, stops power and makes no blind return. Existing page failure
lock blocks further jobs after scan/feed failures until restart and manual origin
restoration. There is no encoder/home/limit feedback. PAGE_RESULT is not ink proof.
Paper feed speed and blank-band CPU rendering are unchanged in this revision.

## One target test

Update/rebuild BK only, HC32 unchanged. Confirm APP_SERVICE_CMY_BIDI_V3. Use the
established left origin and send the same six-bar CMY image once. Observe smooth
motion without binding, printing in both directions, then retain the photo and
complete BK/HC32 logs. The first target trace determines real traversal speed and
any reverse registration error. Do not start a long idle study before review.

## Validation

ASan/UBSan harness executes the ACTUAL page renderer/controller and new step
engine with a fake physical carriage. Every SPI byte is checked against expected
CMY nozzle routes at its absolute physical X, in both directions, across all42
bands including the final8 rows. Verify alternating endpoints, no service firing,
1180 columns per band, cumulative feeds, two successive jobs, final head-OFF park
for an odd scan count, idempotent park, SPI/ON/OFF/feed failure and no blind retry.
Original192/48/24-row, combined/split-CMY page orchestration and legacy service
window regressions pass. SDK startup/worker/protocol/motor objects compile. These
are software checks, not a full firmware link or target motion/ink verification.
