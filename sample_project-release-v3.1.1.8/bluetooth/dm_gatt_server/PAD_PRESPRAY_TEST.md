> Current V47 adds a dedicated HC32 prespray padding profile. See
> PRESPRAY_FIRE_PROFILE_TEST.md; both MCUs must be updated. Dose/mapping below
> remain as V46. This is not a calibrated2.2us or preheat mode.

# RTC-based sponge-pad prespray: V46 single Domain/Section comparison

Normal branch: codex/printer-startup-load-prep. Image printing stays24-row S1,
with original size, feed, carriage speed/stroke and HC32 firmware unchanged.
Prespray runs at boot and before each validated full-page job, over the
user-confirmed sponge-pad coverage. No TSR, preheat, short-CE mode, voltage
change, or2.1/2.2us timing change is introduced. The legacy long boot cleaning
routine remains disabled. No background idle firing/new resume callback is added;
a print following wake uses the ordinary pre-page policy.

## RTC dose

The last successfully completed ink-request scan/flash is tracked with64-bit
AON RTC microseconds in the serialized worker/boot context. This is requested
firing, not measured droplets. Successful nonblank image scans, prespray batches
and manual maintenance update it; blank carriage scans do not. No timestamp is
persisted over power loss. Unknown time or RTC rollback selects the long dose.

| Idle | Requests/nozzle | Total requests | Cache rows | Playback + OFF pauses |
| --- | ---: | ---: | ---: | ---: |
| <3minutes |5|3000|1|~0.8s|
|3..<15minutes |50|30000|3|~2.6s|
|>=15minutes / unknown boot |200|120000|10|~8.8s|

Transfer, positioning and return times are additional. A captured dose is not
reduced midway when successful batches update the clock. Partial/failed batches
are never retried automatically.

## V46 pattern versus V45

V45 did NOT set D1..D6 all HIGH. Its24-request groups used at most4 Domains in
one transport column and at most1 request for any address. Therefore a1.8A
VP droop cannot be inferred from the old pattern; voltage remains unmeasured.

V46 explicitly assigns every nonempty transport column to ONE Domain and ONE
Section. For D1 through D6, it visits S1 through S4. Each25-address section is
split into24+1 requests, making48 groups/round.20 rounds fit a1180-column row.
No data byte has more than one Domain bit; all occupied slots in a column have
the same section; each address has at most one request. At most24 requests are
spread over addresses in the whole column, not fired600 simultaneously.
This isolates Domains/Sections without claiming a particular current reduction
ratio or stable15V. It does not substitute for probing VP at the connector.

Each group receives at most8 consecutive-column shots (673us apart,~1.49kHz)
then the next group runs. Existing HC32 address/S sequencing,25+1 zero stop,
Ax/Dx reset and minimum60us completed-column idle guard remain unchanged. The
per-nozzle repeat interval exceeds the user-cited55.56us minimum. The next
cache row is uploaded only after100ms of head-OFF recovery, except after the
last row.20-round chunks give5,20+20+10, or20*10. Total dose is unchanged.

## Position, ownership and fault handling

Use the established wiper-side starting position and connected active-LOW
GPIO24 reference. Head-OFF seek to the sponge-side optical reference, retract
374steps to PRINT_RIGHT (previously confirmed inside pad coverage), stop and
release TIMER4, then fire the finite stationary row(s). No image or paper feed
runs while positioning. Missing/early/stuck optical input or motion errors reject
firing; this revision does not change the intermittent GPIO24 issue.

CRC, READY, finite count and timeout checks remain. Faults stop and block the
image job, without blind return or retry. Afterwards command head OFF, park at
the wiper, and proceed. At boot this replaces the old reference-only round trip,
before BLE. On pages it follows parsing/allocation and precedes the unchanged
wipe and six-leg10.8mm loading. No extra wiping is added by prespray itself.

## Checks and first comparison

Host tests compare all600 routes against the independent nozzle CSV, exact
doses and burst ordering; assert single Domain/Section and one bit/address;
exercise RTC boundaries/rollback, pauses, failure cleanup, boot and page ordering.
ARM SDK object checks cover actual integration. These do not establish real
voltage, pulse width, ink delivery or recovery from dried ink.

Keep the current head condition and print the same CMY frame after one boot
prespray. Capture PRESPRAY_BEGIN/BATCH/DONE and HC32 logs. BEGIN identifies
domain_mode=D1_to_D6 one_D_one_S_per_column row_rounds=20; boot identifies
APP_SERVICE_CMY_PAD_DOMAIN_PRESPRAY_V46. STOP means the spit did not complete;
DONE confirms protocol requests, not actual ink flow. Do not simultaneously
change tFire/VP or wet the head for this comparison.

## Restore points

backup/before-domain-prespray-20261123:6609d0e (V45 burst pattern).
backup/before-burst-prespray-20261123:5d8a642 (V44 sparse pattern).
backup/before-auto-prespray-20261123:344ba4c (S1 without automatic spit).
Save local edits and build a separate branch from the desired fetched tag.
PRINTER_PRESPRAY=0 restores the original boot/image flow. The temporary motor
branch remains separate. HC32 is still79ba462, with no new firmware required.
