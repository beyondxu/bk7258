# V30: another6.45% carriage cruise increase

Requested another small speed increase to approach the sample machine.

- STEP66us ->62us:6.45% faster cruise than V29, approximately94% faster than the original120us cruise. Head-OFF carriage cruise also62us.
- Ink column542us ->509us, above the current500us protocol minimum. Existing HC79ba462 supports this; no further HC source update is needed, but earlier600us-minimum HC is incompatible.
- Column period rounds985*62/120, retaining physical pitch within0.0164% of baseline (~0.008mm across50mm). Mapper pitch4390 hundredths of a micrometre follows the physical pitch.
- Image lead396/ramp346 steps retains50-step cruise clearance. Maintenance ramp400 and initial period1000us unchanged. Print10473/service11221, insets374, paper loading6*702, REF, microstepping and electrical firing pulse widths unchanged.

Validation: actual BK cached transport/timer simulations at62/509 cover1180 columns within cruise, both directions, optical stop/reversal, preparation feeding, blank motion and final park. Page-controller tests and six SDK ARM object compilations pass. Existing HC tests already exercise500us playback and its20us overrun guard. This does not prove the target's actual execution time or physical motor behavior.

Keep HC's20us timing protection enabled. At509us, the checked per-column execution must stay below489us (97800 cycles at200MHz). On the first same-image hardware comparison, inspect [HC32 LINE_DONE] state/error, fired=1180/1180, max_fire_cycles and max_late_cycles alongside carriage smoothness/noise. A timing error should be investigated rather than bypassed. Confirm HC startup includes speed_trial_v3 min_period_us=500 before using this BK build.
