> Active V8: reduce both margins2mm after the extended stroke operated service
> mechanisms and interfered with paper grip. Margins1964->1590 steps (~8.5mm),
> full span11408->10660 steps (~57mm), print segment7480 unchanged. Manually
> establish the LEFT origin2mm right of the V7 mark to shrink both ends while
> retaining the image position. Startup APP_SERVICE_CMY_BIDI_V8.
> Keep RIGHT230us/250-step ramps and LEFT260us/400-step ramps,24-row bands,
> fractional paper feed, reverse registration and head-OFF service regions.
> Tests update all absolute physical positions and pulse limits accordingly.

> Active V7: the user reports residual noise specifically on leftward return.
> Keep RIGHT LOW230us, start1000us and250-step ramps. Use LEFT LOW260us,
> start1200us and400-step outer ramps, including the final head-OFF park.
> Both ramps remain within1964-step margins. Startup APP_SERVICE_CMY_BIDI_V7.
> Pixel/nozzle routes, column count, feed, geometry and HC32 energy are unchanged.
> Direction-specific pulse/ramp and physical-column tests pass; actual sound and
> reverse print registration must be observed. Earlier V6/V5 notes follow.

> Active V6: the user reports V5 jumping/noise is much improved and requested
> a modest speed increase. Cruise LOW260us becomes230us; HIGH4us, start LOW1000us,
> outer250-step ramps, fixed pulse delays and no catch-up remain. Startup marker
> APP_SERVICE_CMY_BIDI_V6. The baseline V5 analysis below is retained for context.
> Nominal cruise step rate rises from1/264us to1/234us (~12.8%); actual image
> traverse gain is smaller due to unchanged synchronous SPI/HC32 waits. This does
> not alter ink pulse width, voltage, image point count, feed or service geometry.
> The updated tests check exact230us LOWs/ramps and the same adversarial waits.
> Hardware smoothness at the new speed still needs one target observation.

# Fixed STEP delay comparison after V4 carriage jumping

The user reports jumping and abnormal sound. V4's target log has blank passes
around2.106s and an image column wait as long as7157us, with other waits813us.
The six-step average budget did not guarantee even adjacent pulse spacing.
V4 also explicitly slept1ms every512 steps on blank passes, adding periodic
motion interruptions. Do not infer physical travel or correct alignment from
the successful software pulse count or PAGE_RESULT.

## V5 change

Retain actual CMY24-row bands, alternating directions including blank bands,
1964-step non-firing margins, full11408-step stroke, same manual LEFT origin,
reverse206-step lead/domain registration, page-owned boost and0.21s-class fast
small feed. Keep HC32 pulse widths, voltage and image point counts unchanged.

Remove six-step time compensation and all RTC-derived per-step delay adjustment.
Remove explicit RTOS task sleeps from the carriage stroke. Every commanded
pulse uses HIGH4us plus LOW260us in cruise, with the same1000-to260us ramps over
250 steps at the two outer endpoints. There is no shortened pulse to recover
communication time. This slows nominal motion versus V4 and prioritizes checking
mechanical stability; it is not the sample printer's speed.

Synchronous SPI/HC32 waits and scheduler interrupts CAN STILL extend STEP-low
intervals. V5 does not claim constant velocity, no missed physical steps, or that
all reported jitter is fixed. The measured long waits require eventual separation
of pulse generation from blocking communication if stable fast movement cannot
be achieved. This revision deliberately avoids deploying an unverified timer/PWM
pin remap or interrupt pulse engine during a physical-jumping regression.

BIDI_SCAN retains communication timing and replaces step_overruns with
long_column_waits (number of column calls >1000us). Failure stop/position lock and
no blind returns are unchanged. Preflight errors still never power hardware.

## One target observation

Update/rebuild BK only and confirm APP_SERVICE_CMY_BIDI_V5. Since the prior run
may have lost steps, power OFF and restore the established manual LEFT origin
with clearance before startup. Send the same image once and first observe its
initial blank traversals for smoothness and normal sound. If jumping persists,
stop the run rather than continuing the whole page or repeating prints. Retain
BIDI_SCAN logs and note whether disturbance occurs on blank or ink-enabled scans.
No new wipe, head swap or extra firing is required for this motion comparison.

## Validation

ASan/UBSan harness executes actual renderer/controller/pulse engine and verifies
every forward/reverse SPI byte against physical X, all42 bands including blanks,
page-owned power, fractional feeds, partial final band, two jobs, odd final park,
and SPI/head/feed failures without blind return. It asserts HIGH4us, exact ramp
LOW values and cruise LOW260us for every commanded step, and at least260us low
before the next pulse. No task-delay mock is provided, so reintroducing explicit
stroke sleeps fails compilation. A coarse32us diagnostic clock and variable
350/813/7157us mocked column waits cannot shorten later pulses. Nominal mock image
traversal is about3.7s; real timing and mechanical stability need target checks.
Related SDK startup/worker/protocol/motor objects compile. No full firmware link
or physical proof is implied.
