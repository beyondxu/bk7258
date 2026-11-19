> Superseded by [V27: another20% cruise increase](CARRIAGE_SPEED150_TEST.md).

# V26: increase carriage cruise speed25%

The user requested a faster carriage before continuing the faint-print investigation.

- TIMER4 STEP cruise period:120us ->96us (25% more pulses/second).
- HC cached-line column period:985us ->788us; exact same ratio preserves horizontal dot pitch and image dimensions.
- Existing HC line protocol minimum period is600us;788us is accepted by the unchanged HC firmware. BK sends the period in the existing line ticket.
- Head-OFF carriage cruise also uses96us, including boot, wipe, loading and parking. Paper motor settings and feed increments are unchanged.
- Stroke lengths remain10473 print /11221 service; optical endpoint/insets are unchanged.
- Existing ramp lengths and1000us initial period remain; total job time is not reduced by exactly20% because acceleration, paper feeding and data preparation remain.
- Electrical firing pulse width, VP and microstepping configuration are unchanged. This change does not establish that the idle/faint-print problem is fixed.

## Noise interpretation

The sample's quicker, quieter motion is an observation, not an electrical diagnosis. Possible causes include current regulation/REF differences, different effective microstep configuration, timing jitter or motor resonance, and the mechanical load at the wiping station. Previously reported REF was1.8V on the sample vs3.3V on our board; the present board voltage must be rechecked before assuming that difference persists. Compare with the actual chip's current-limit formula, sense resistance and winding rating; do not infer current directly from voltage alone. Faster movement can shift resonance but does not guarantee quieter motion.

## First comparison

Print the same image once using this build. Compare forward and return noise and smoothness with V25; if vibration, shortened travel or displaced printing appears, stop testing this speed. Preserve the same head and paper and report the `[BUFFERED_SCAN]` lines, especially `cruise_step_us`, `cruise_max_gap_us`, `pulses` and `columns`. Do not change REF or spray energy during this speed comparison.

## Validation

Actual cached-transport/timer simulation tests both directions,1180 timed columns entirely in the cruise region, unchanged physical pitch/geometry, optical stop/reverse, concurrent preparation feed, blank motion and final park. It also checks96*985 ==788*120. Six SDK ARM objects compile successfully. Physical motor noise, missed steps and ink ejection still require the hardware comparison.
