# V31: match the sample STEP waveform estimate

The user requested matching the sample after supplying its oscilloscope screenshot. At50us/div, successive rising edges are approximately82us apart; HIGH is approximately27us and LOW55us. These are screenshot estimates, not exported scope measurements.

- Cruise STEP period62us ->82us (~12.2kHz); STEP HIGH4us ->27us, nominal LOW55us.
- Applies to the timer-driven U4 paths in both directions: printing, blank scans, boot, wiping, loading preparation and final parking. The non-buffered legacy fallback and U5 feed pulse generation are unchanged.
- HC column509us ->673us. The existing updated HC79ba462 supports this period, so no HC source/flash update is required if already installed.
- Column673us rounds985*82/120; baseline pitch error -0.0124% (~0.0062mm/50mm). Mapper pitch4388 hundredths of a micrometre follows the physical pitch.
- Lead398/image ramp348, retaining50-step cruise clearance. Head-OFF ramp400 and initial period1000us remain unchanged. Sample acceleration/reversal has not been measured from the short scope capture.
- Print10473/service11221, insets374, paper6*702, REF, microstep pin settings and electrical nozzle firing pulse width unchanged. STEP HIGH refers only to the motor signal.

This sets firmware targets; GPIO call overhead, delay calibration and IRQ timing mean matching the physical waveform still requires a scope check. Capture our STEP at50us/div in the same middle-of-stroke region, in each direction. Compare actual rising-edge spacing, HIGH/LOW widths and interval consistency with the sample. Do not infer actual edge jitter from the coarse (~32us) RTC interval log alone.

Validation: deterministic actual transport/timer tests assert27us HIGH on every pulse and82us rise-to-rise in cruise, preserving nominal55us LOW. Tests also cover1180 columns within cruise, both directions, interrupted motion, optical stop/reverse, preparation feeding, blank scans and final parking. An already-started HIGH pulse completes before falling even if OC3 stops future pulses. Page-controller tests and six BK SDK ARM object compilations pass. No claim of measured hardware waveform equality or quieter motion is made by the simulation.
