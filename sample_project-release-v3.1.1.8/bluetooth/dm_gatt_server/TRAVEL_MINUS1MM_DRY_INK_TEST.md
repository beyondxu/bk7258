# V25: both single strokes minus1mm

The user requested shortening both V24 strokes by1mm. Using the observed7480 pulses /40mm conversion, subtract187 pulses once per single stroke (not once at each end).

| Stage | V24 | V25 |
|---|---:|---:|
| Image single stroke | 10660 | 10473 |
| Boot / paper-loading / OC3-to-final-wiper single stroke | 11408 | 11221 |
| Service-to-print inset at either end | 374 | 374 |

The service/print length difference remains748 pulses (~4mm). GPIO24 LOW remains the RIGHT stop reference; the step count bounds a missing-switch search. The final wiper endpoint is shared, without another added retreat. Physical station contact and collision clearance still require observation; step counts do not verify either.

The shortened image stroke leaves398 lead steps. Image ramps are therefore348 steps, keeping50 steps of cruise clearance before/after the1180 timed columns. Non-image motion retains400-step ramps. The120us cruise STEP period,985us column period, image pitch/dimensions and six702-step paper-loading increments are unchanged.

## Dry-nozzle investigation: first observation only

Known evidence: prints may be blank after idle and recover after manual wiping. This supports a nozzle-face/ink-delivery startup issue, but does not prove dried ink alone; wiping also changes the surface meniscus and can restore wetting or remove pooled ink. The same frame does not prove that the current counted wiper position actually engages the nozzle face.

Current firmware does two20mm head-OFF wipe round trips before the six head-OFF paper-loading legs, then prints, finally parking at the wiper. It does not command a separate suction pump, sealed cap, idle maintenance firing or timed moisture control. Parking near a blade or sponge does not itself establish an airtight seal. The sample reportedly parks at its blade, so do not change parking sides on that assumption.

First observation: during the existing two short preparation wipe round trips, establish whether the blade actually passes across the nozzle face and removes ink. Report contact/no-contact/unclear. Do not alter firing energy, add ink ejection over the non-paper service region, or change multiple preparation parameters while obtaining this observation. Any later idle comparison should use the same head, image and wipe history, and record the elapsed idle time.

## Validation

Actual buffered transport/timer simulation checks the new10473/11221-pulse lengths, both directions, all1180 columns occurring within the constant-speed zone, optical stop/reverse handling, unchanged4212 paper steps and final parking. Preparation and page-controller simulations and six SDK ARM object compilations are also run.
