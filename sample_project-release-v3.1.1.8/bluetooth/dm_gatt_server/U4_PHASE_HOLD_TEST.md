> Current cached-line timer version retains this phase fix; use
> CACHED_LINE_TIMER_TEST.md for the paired BK/HC32 update and test.

# U4 carriage phase preservation (V9)

User priority is carriage-motor sound; do not change U5 paper-feed speeds,
microsteps, pulse count, ink algorithms or carriage timing in this test.

## Datasheet evidence

Uploaded ATD5984-S402-V1.3, table1, printed page5 lists columns MS3/MS2/MS1.
111 and011 both select1/16. Therefore the active direct-HIGH MS1/MS2/MS3=111
and enum MODE_1_16_STEP=3 (MS1/MS2/MS3=110) both correctly select1/16. Merely
finding different encodings was not evidence of a wrong microstep setting.
Do not substitute an A4988 truth table or rescale calibrated travel here.

Printed page6 says nRESET LOW returns the translator to its initial state and
disables outputs. Printed page7 says SLEEP wake presets outputs to HOME and
requires1ms recovery. nENABLE controls output FETs independently of translator
inputs. This establishes reset/sleep phase loss; it does not prove all observed
acoustic noise is caused by it.

## Concrete software issue and correction

The previous fast small-band paper-feed helper called bk_motor_set_state(1,0),
which drives U4 ENABLE high, RESET low and SLEEP low at every band boundary.
The next scan called carriage_wake_on_powered_rail again. Thus page-owned boost
alone did not preserve the carriage driver's electrical microstep position.
Repeated re-energization into HOME can cause a kick/click or position disturbance.

Remove the U4 disable/reset/sleep call from fast paper feed. Do not re-run U4
wake on every image/blank scan or final park. Initialize/wake U4 once per page,
keep MS111, RESET/SLEEP high and ENABLE low through scans and short paper feeds,
and disable/sleep it only at page completion or abort. The motor holds its
endpoint while U5 feeds. This hold lasts only for the bounded page, not indefinitely.
HC32 head power remains OFF during paper feed and non-paper carriage travel.

Retain V8 margins1590 steps, span10660, forward LOW230us/250-step outer ramps,
reverse LOW260us/400-step outer ramps, start LOW1000/1200us, fixed HIGH4us,
actual24-row CMY,396/397-step feeds and reverse registration. No change to U5's
own control sequence or HC32 spray energy. There are still synchronous SPI waits
and mechanical/driver-current factors; do not claim silent cruise or constant
velocity from phase preservation alone.

## One observation

Update/rebuild BK only; HC32 unchanged. Confirm APP_SERVICE_CMY_BIDI_V9 and
U4_hold_phase_MS111. Start at the established V8 left origin. Send the usual
image once and first listen to the initial blank traversals and reversals. No
manual wiping is required to judge carriage sound. Compare the endpoint restart
kick separately from noise during the middle of travel. If motion binds/jumps,
stop instead of repeating. Retain BIDI_SCAN/BIDI_FEED logs and the observation.

## Validation

Actual renderer/controller/motion ASan/UBSan tests still cover physical column
positions in both directions, all42 bands, page-owned power, blank movement,
final partial band, repeated jobs, odd final park and SPI/power/feed failure
cleanup. The fake device deliberately provides no per-scan carriage-wake helper,
so reintroducing a wake there fails compilation. Actual U5 pulse-loop tests count
U4 state changes and assert fast feeds neither disable U4 nor reinitialize GPIO
or cycle boost. All existing feed pulses and limits are preserved. SDK startup,
worker, protocol and motor objects compile. Target acoustic improvement, supply
behavior during held carriage plus paper feed and actual position remain unverified.
