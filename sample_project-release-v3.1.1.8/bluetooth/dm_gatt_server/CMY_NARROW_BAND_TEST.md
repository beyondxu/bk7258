> Historical single-direction comparison. Active firmware now uses fast alternating
> directions; see CMY_BIDIRECTIONAL_TEST.md. Band/feed details below still apply.

# CMY narrow-band comparison

Video of the sample printer shows frequent traversals and paper advances, but
does not establish exact nozzle-row count or firmware parameters. This test
uses24 image rows per band instead of192. It is a deliberate comparison value,
not a claimed copy of the sample printer. It retains single-direction printing;
reverse printing and reverse color registration are not implemented here.

## Data and motion

The actual APP page decoder/resizing and CMY pixels remain unchanged. For each
band, take the next24 page rows and place them on physical nozzle rows0..23
of each color (with the existing first-nozzle/reverse configuration). Horizontal
registration and all1180 columns remain. This changes the nozzle subset; it is
not repeated exposure or a reduced column count. Each resized page bit is
scheduled once, including bands crossing previous192-row boundaries and the
last partial band. Blank bands skip scans but preserve paper advance.

Keep the192-row feed calibration3170 pulses. Each24-row increment uses396 or397
pulses, with cumulative integer division to avoid rounding drift. Every8 feeds
sum to3170. Nominal physical band height24*0.04233=1.01592mm, still subject to
paper-feed calibration/slip. The earlier suggested2975 calibration is not used.
For the captured1119x992 test page there are42 bands/41 feeds (16246 pulses),
with10 nonempty scans; native192-row mode had6 bands/5 feeds and5 nonempty scans.
Final paper position changes by the different trailing-band padding.

Keep the1964-step margins (~10.5mm each),11408-step full stroke, established
manual left origin, print/service speeds,2000ms head settle, HC32 pulse widths
and voltage. Head OFF on maintenance travel and return. Do not infer clearing,
wiper contact, sealing, physical position or ink from pulse/column counters.
With more nonempty scans and retained empty returns, this comparison can take
several minutes; it is not yet the sample printer's throughput. Dense pages can
take substantially longer. Stop after one six-bar page for review.

## One test

Update/rebuild BK only; HC32 unchanged. Confirm APP_SERVICE_CMY_V2 at startup
and PAGE_BAND_PLAN rows=24 in the job. From the already established left origin,
send the same six-bar CMY image once. Keep the existing head state rather than
adding a new wipe to this comparison. Retain paper photo plus BK/HC32 logs to
PAGE_RESULT; compare each line's completeness and the previously split cyan.
No non-paper firing, automatic retries, or extra black prefix is added.

## Validation

Controller tests cover192/48/24-row bands and combined/split CMY passes. They
compare every emitted column against independently constructed expected routes,
including the last8-row band, cumulative feed counts, no feed between color
passes, OOM recovery, and failure locking after scan/short-feed errors. Existing
full-page parser/layout and service-window pulse tests pass. The previously
captured actual APP BIN retains exactly23099/23373/22724 emitted CMY points over
24-row bands, matching its resized source once. SDK startup/worker/protocol/motor
objects compile. Full firmware linking and hardware behavior need target checks.
