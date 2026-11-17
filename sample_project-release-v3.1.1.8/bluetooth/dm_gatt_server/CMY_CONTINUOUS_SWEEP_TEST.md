# Continuous-sweep CMY workflow (V4)

The V3 target trace shows image traversals around2.23s, communication around
0.34s/1180 columns, a396/397-pulse feed around1s and rendering around0.34s per
band. Blank bands skipped scans, causing long accumulated endpoint waits. The
user requested the sample machine's more even motion, smaller endpoint pauses,
slower image-center motion and continuing scans through blank page regions.

## Motion and timing

Every24-row band now makes a full11408-step traverse, including blank bands.
RIGHT/LEFT alternate on every band. For the captured1119x992 page this is42
traversals and41 fractional396/397-pulse paper feeds. Blank traversals send no
image columns and never enable the head. Actual image bits remain scheduled
once, not ten repeated fires per line. Video does not establish a ten-pass mask
or justify multiplying nozzle dose.

One full-stroke loop replaces the three independent ramped legs. Only the outer
250 steps accelerate/decelerate (600-to170us nominal step period); the1964-step
maintenance/image boundaries have no intermediate ramps or stops. Enable the
head at the original image boundary and turn it OFF at image exit. Reverse
206-step versus forward200-step lead and reverse domain compensation remain.
Keep overall1964-step margins,7480-step image length and the manual LEFT origin.

Cruise average budget is170us/step. Each image column shares a1020us budget
across six steps, including synchronous SPI/READY latency, and distributes the
remaining delay. HIGH stays4us, LOW is never shorter than80us. If a handshake
exceeds the available budget, slow down and count overruns; do not burst to
catch up or skip READY. With the observed roughly290us average communication,
this moderates the V3 image-center speed while making maintenance and image
AVERAGE speeds closer. This is software pacing with blocking SPI, not a hardware
constant-velocity guarantee. Short microstep jitter and HC32 control-boundary
waits remain. It does not claim the sample's0.67s traverse.

Own the shared boost for the whole page, starting it once and turning it OFF at
success/failure. Keep HC32 head power OFF while parked, feeding or traversing
maintenance zones. U4 is disabled during feed, then woken without GPIO reclaim
or cycling the shared boost. U5 small feeds (at most500 pulses) use HIGH4us,
LOW800-to400us with64-step outer ramps, same microstep GPIOs including MS3=17,
and no interrupt masking. Host clock predicts about0.21s per396/397-pulse feed.
Real paper motion/slip still needs measurement;3170 pulses/192 rows stays the
calibration. No simultaneous paper motion and image firing.

## Rendering, failures and evidence

Render only requested rows directly into route-compensated output columns.
Do not scan192 nozzle rows per output column or regenerate unaffected zeros.
Validate routes before writing outputs; retain API bounds and full-page parser.
Reference tests compare all bytes with the old scratch+mapper-column algorithm
for192/48/24 rows and both directions, on asymmetric test data and a captured
actual APP BIN. All original CMY resized point totals are conserved.

BIDI_SCAN includes blank flag, direction, column count, time, communication time,
overruns and counted endpoint. BIDI_FEED includes actual feed function duration.
Any scan, handshake or short-feed error aborts the owned power session, invalidates
counted position and stops without blind return. Preflight failures do not power
hardware. Final parking is head-OFF, and needed only after an odd number of
traversals. Pulse counts are not actual carriage position or ink proof.

## One target observation

Update/rebuild BK only; HC32 unchanged. Confirm APP_SERVICE_CMY_BIDI_V4. Starting
from the same established left mark, print the same six-bar image once. Observe
that blank regions also scan, endpoint pauses shrink, and no extra slowdown occurs
at image/service boundaries. Retain the photograph and full BK/HC32 logs through
PAGE_RESULT, especially BIDI_SCAN and BIDI_FEED. Check physical reverse alignment
and that paper did advance correctly before further tuning. No added manual wipe
or long idle study is required for this workflow comparison.

Validation includes actual renderer/controller/motor engine under ASan/UBSan,
physical forward/reverse byte positions, all42 scans including blanks, final8-row
band,1020us column cadence with350us mocked communications, page-owned power,
two successive pages, odd-scan parking and fault/short-feed stop without blind
return. Actual U5 fast pulse loop checks396/397 counts, timings, invalid arguments,
no boost cycling and MS3=17. Legacy page/service and parser regressions pass.
SDK startup/worker/protocol/motor objects compile; full link and real operation
remain target checks. The photo's weak magenta is not diagnosed as fixed by this
motion change; logs/data alone cannot establish actual ejection.
