# V2: manual left-service origin, one rightward non-firing stroke

The user changed the requested origin after the initial four-leg trial. V2
replaces that itinerary: start at the LEFT service position, move RIGHT11220
steps once, stop on the RIGHT, disable U4/boost. No initial left move, no reverse,
no automatic return. The previous old-origin/four-leg instructions are obsolete.

## Geometry

User calibration: old7480 steps is approximately40mm. Extra10mm each side is
1870 steps, so endpoint span is11220 steps (~60mm). New coordinate0 is the LEFT
maintenance position, not the original print start. The legacy7480-step segment
would be between coordinates1870 and9350. These are approximate counted offsets,
not calibrated paper edges or measured carriage positions.

No home switch/encoder procedure exists in this path. Power OFF first and place
the carriage at the known left maintenance position, leaving normal mechanical
clearance. Do not drive into a hard stop to establish zero. A further trial
requires returning to that position with power off before retriggering. Watch
for binding or a hard stop and stop power if necessary. Source pulse counts
cannot detect actual lost steps, collision or correct mechanical contact.

## One target action

Flash BK; leave HC32 unchanged. After placement and whole-machine startup,
confirm CARRIAGE_SERVICE_LEFT_START_V2 and link ready=1. Send the usual APP image
once. This triggers only one rightward stroke (~12seconds at the test speed).
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
exact11220 pulses, direction always1, monotonically0..11220, about12seconds,
head OFF before any motor edge, no feed/firing, bounded arguments, link/OFF
failure blocking, partial/error stop without return, one-shot behavior. BK SDK
source objects compile. Target-side distance, endpoints and mechanism operation
remain to be confirmed before integrating any image or maintenance firing.
