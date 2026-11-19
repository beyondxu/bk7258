> Superseded by [V28: another8.1% cruise increase](CARRIAGE_SPEED162_TEST.md).

# V27: another20% carriage cruise increase

User request: continue increasing carriage speed, without changing REF or maintenance geometry.

- STEP cruise96us ->80us:20% faster than V26,50% faster than the original120us cruise.
- Column788us ->657us, rounded from985*80/120. Above the unchanged HC protocol minimum600us; no HC update required.
- Whole-us rounding changes horizontal pitch by0.051% (~0.025mm across50mm). Mapper pitch4389 ->4391 hundredths of a micrometre follows that small physical change for nozzle-offset compensation.
- Image lead395 steps; image ramp345 steps maintains50-step clearance before timed firing. Service ramps remain400 steps and initial period1000us remains unchanged. Total job duration does not scale exactly with cruise speed.
- Print10473 / service11221, insets374, six702-step paper increments, REF, microstep mode and electrical firing pulses unchanged. Head-OFF carriage cruise uses the same faster80us period.

Actual cached transport/timer simulations check forward/reverse timing, all1180 columns within cruise, blank motion, optical stopping/reversal, unchanged paper increments and final park. Page-controller tests and six SDK ARM object compilations pass. Hardware smoothness, missed steps, noise and ink output require the user's same-image comparison. This version is a speed trial, not a claim to resolve the idle/faint-print issue.
