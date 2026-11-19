> Superseded by [V30: another6.45% carriage cruise increase](CARRIAGE_SPEED194_TEST.md). HC79ba462 remains required.

# V29: another12.1% carriage cruise increase; paired HC update

Pair with HC commit79ba462 on codex/printer-cached-line-timer. This revision requires updating both controllers, HC first; the old600us-minimum HC rejects542us and will not provide the new speed.

- STEP74us ->66us (12.1% faster); ink column607us ->542us.
- Header minimum period600us ->500us, matching HC byte for byte. Existing protocol layout unchanged.
- Column542us rounds985*66/120; baseline physical pitch error+0.046%, ~0.023mm per50mm. Mapper pitch4391 hundredths of a micrometre follows the physical pitch.
- Lead395/image ramp345, still50 steps of cruise clearance. Head-OFF carriage cruise also66us. Start period1000us, maintenance ramps400 steps unchanged.
- Print10473/service11221, insets374, paper loading6*702, REF, microstepping and electrical firing pulses unchanged. Total job time does not scale exactly with cruise speed.

HC's existing20us deadline/overrun checks remain enabled. At542us its measured per-column execution must stay below522us and lateness within20us. Host tests use synthetic execution durations and do not prove the actual hardware's available timing margin. First print the same image and capture [HC32 LINE_DONE], particularly state/error, fired=1180/1180, max_fire_cycles and max_late_cycles, alongside [BUFFERED_SCAN]. At200MHz, divide cycles by200 for microseconds. A PL_TIMING failure stops the row; it is not recovered through catch-up firing.

Validation: real BK cached transport/timer simulation at66/542,1180 columns within cruise in both directions, optical stop/reverse, concurrent paper feeding, blank scans and final park; page-controller tests; six BK SDK ARM objects compiled. HC cache bounds and playback tests include500/542us completion and exact522us overrun rejection, and full GCC link passes. Keil rebuild/hardware noise, missed steps and ink output remain for user verification.
