> Historical dry-run mode is inactive. Current shared geometry adds94 steps
> per end:1590-step margins (~8.5mm),10660-step span (~57mm).
> Use SERVICE_BLACK_YELLOW_PRINT_TEST.md for the current printing procedure.

# V4: manual left-service origin, one rightward non-firing stroke

The user changed the requested origin after the initial four-leg trial. V4
replaces that itinerary: start at the LEFT service position, move RIGHT10472
steps once, stop on the RIGHT, disable U4/boost. No initial left move, no reverse,
no automatic return. The previous old-origin/four-leg instructions are obsolete.

## Geometry

User calibration: old7480 steps is approximately40mm. Extra8mm each side is
1496 steps, so endpoint span is10472 steps (~56mm). New coordinate0 is the LEFT
maintenance position, not the original print start. The legacy7480-step segment
would be between coordinates1496 and8976. These are approximate counted offsets,
not calibrated paper edges or measured carriage positions.

No home switch/encoder procedure exists in this path. Power OFF first and place
the carriage at the known left maintenance position, leaving normal mechanical
clearance. Do not drive into a hard stop to establish zero. A further trial
requires returning to that position with power off before retriggering. Watch
for binding or a hard stop and stop power if necessary. Source pulse counts
cannot detect actual lost steps, collision or correct mechanical contact.

## One target action

Flash BK; leave HC32 unchanged. After placement and whole-machine startup,
confirm CARRIAGE_SERVICE_LEFT_START_V4 and link ready=1. Send the usual APP image
once. This triggers only one rightward stroke (~7.3seconds at the test speed).
No paper feed, head-ON or firing-column calls occur anywhere. The head-OFF
command must be acknowledged before boost/motion. A one-shot latch blocks an
additional attempt until restart, including after a partial/error result.

Capture SERVICE_BEGIN/LEG/POSITION/END. Observe whether both maintenance parts
are crossed and where the carriage stops. No automatic return is made on either
success or error. This is still the mechanical verification mode, not printing.

## Meaning of "printing on both sides"

Bidirectional printing means image firing during both left-to-right and
right-to-left scans over the paper; it needs direction-dependent image/nozzle
registration. Maintenance spitting means controlled firing into a confirmed
waste-ink receiver outside the paper. Wiping/capping is a separate mechanical
operation and does not itself establish that either service end should fire.
The user was asked which observation they meant. Until clarified, no reverse
printing or out-of-paper maintenance firing is enabled. Public webpages were
blocked by environment network policy; no model-specific two-end behavior was
verified. The normal print code remains in history and inactive in this dry run.

## Verification

ASan/UBSan tests run the actual service loop/controller with a fake clock:
exact10472 pulses, direction always1, monotonically0..10472, about7.3seconds,
head OFF before any motor edge, no feed/firing, bounded arguments, link/OFF
failure blocking, partial/error stop without return, one-shot behavior. BK SDK
source objects compile. Target-side distance, endpoints and mechanism operation
remain to be confirmed before integrating any image or maintenance firing.

V3 shrinks each extension by2mm at the user's request. Non-paper-end firing is
explicitly deferred; this firmware remains head-OFF throughout. Source filename
retains the original10mm trial name, while current constants/banner specify8mm.
Place the start at the new8mm left-maintenance mark, about2mm to the right of the
old10mm mark. With that origin, the far endpoint is also2mm inward. Merely keeping
the old origin and shortening the stroke would not shrink both physical ends.

V4 raises only the service-test cruise speed: STEP LOW1000us becomes600us.
The initial/final LOW3000us and250-step symmetric ramps, HIGH4us,10472-step
stroke and left-origin/right-only/one-shot behavior are unchanged. RTC-model
runtime drops from11.34seconds to7.25seconds including task yields; hardware
elapsed time remains a target-side measurement. Normal printing timing and
head drive are unaffected. Stop and report binding/step loss before increasing
speed further; physical arrival is still unverified by the pulse counter.
