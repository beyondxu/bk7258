> V33 keeps this normal motion profile and slows only short wiping moves: see [slower wipe and idle-color analysis](SLOW_WIPE_IDLE_COLOR_ANALYSIS.md).

# V32: short endpoint ramps, retain sample cruise waveform

The user's scope captures show two STEP patterns: approximately82us period /27us HIGH in the dense capture, and approximately1ms rising-edge spacing in the sparse capture. V31 firmware explicitly used1000us initial/final periods with400-step service ramps and348-step image ramps, so endpoint motion is a plausible explanation of the sparse capture; the screenshots alone do not locate that capture within the stroke. The user reports that sample-machine acceleration/deceleration is not visually obvious and requests the shortest practical transition for this trial.

Final V32 configuration:
- Start/end STEP period1000us ->400us.
- Image and head-OFF ramp lengths ->64 steps per end, approximately0.342mm using7480 steps /40mm. The modeled transition occupies about15ms per end.
- Cruise stays82us, HIGH27us, nominal LOW55us. Electrical nozzle firing widths are unchanged; HC column period stays673us.
- Both directions and all timer-driven U4 stages use these short ramps, including boot, wiping, paper preparation, blank scans and final parking.
- Print10473/service11221, endpoint offsets, final sealing-slot parking, microstep pins, REF and paper6*702 remain unchanged. The reported sealing-slot mechanism is not re-positioned by this change.

This is a short-ramp hardware trial, not a measured match to sample acceleration. Faster transitions require checking actual smoothness, missed steps and repeatable end positions. It is not possible to establish the shortest mechanically reliable ramp from the screenshots or host simulation alone. If the1ms pattern persists in the middle of a stroke with this firmware, it is not the configured cruise and should be investigated with the corresponding movement log and probe location.

Validation: actual cached transport/timer tests enforce400us start,82..400us bounded periods,64-step ramps,27us HIGH and82us cruise rise spacing. Tests also exercise all1180 columns within cruise, both directions, optical stop/reverse, blank moves, concurrent feed, IRQ faults and final parking. Preparation/page-controller tests and six SDK ARM object compilations pass. In the deterministic model, a10473-step stroke now takes approximately0.880s versus V31's approximately1.18s with ink /1.23s blank; these are modeled motion times, not measured hardware or total job duration.

Update BK only on codex/printer-startup-load-prep; keep current HC79ba462. First observe one round trip for smoothness and return-position repeatability, then print the same image if motion is stable. The startup marker includes STEP82, start400 and ramp_steps64.
