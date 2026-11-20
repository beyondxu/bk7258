# V33: slower short wiping moves and idle-color investigation

## Motion change

The user confirmed V32 has no observed jumping, missed steps or return-position shift, and requested slowing only the small pre-print wipe moves. Only printer_buffered_wipe now uses164us cruise STEP (half the82us print/service cruise speed). It remains two20mm out/back rounds,27us HIGH,64-step ramps and400us starting period, head OFF throughout. Boot, the six concurrent paper-loading legs, image/blank scans, final parking and their positions remain unchanged. The wipe profile is reset before normal completion or optical/error recovery; subsequent movement returns to82us. WIPER_LEG logs cruise_step_us=164.

Validation: actual timer/transport simulations assert164us rise spacing throughout wipe cruise, exact excursion/pulse counts, zero nozzle firing, and restoration of82us positioning on normal and fault exits. A dedicated unexpected-optical-stop-during-wipe case verifies normal-speed return. Preparation/printing retain the existing geometry,4212 paper steps,1180 columns at673us and82us cruise. BK SDK ARM objects compile successfully. Wiper contact, cleaning effectiveness and sealing are not established by software counts.

## Evidence and limits

User observations: after about10 minutes idle, cyan still prints while magenta disappears; yellow is deferred. Water on the nozzle face can restore output, and wetting the service sponge helps somewhat. The same head has printed normally in the sample machine. The machines share the mechanical frame, and the wiper-side endpoint places the head into a sealing slot. These observations favor an idle surface/meniscus/maintenance interaction, but do not by themselves prove dried solids, an airtight seal, or normal magenta electrical drive and supply. Sample startup maintenance and head transfer also change conditions, so head transfer is not a single-variable electrical experiment.

A production solution cannot depend on periodic manual water addition to an exposed sponge. Different color rows can fail at different rates because of local seal pressure/leakage, ink composition and wetting, supply impedance/air, or insufficient effective drive/recovery margin. Cyan surviving does not exclude a partial sealing issue; magenta disappearing does not prove an APP bug. Shared VP/CE/S routines do not prove equal physical D-channel waveforms or jetting energy.

## Source findings

- Normal completion parks through printer_buffered_park_at_wiper, then printer_bidirectional_finish calls carriage_verified_power_down.
- That function sets U4 EN HIGH, SLEEP LOW, RESET LOW and disables the motor15V boost: the firmware releases holding torque after parking.
- Therefore one concrete hypothesis is sealing-spring reaction/backdrive after driver release, even if the nozzle initially enters the slot. Actual backdrive/leakage is unmeasured; compare the sample's parked driver state before changing hold behavior. A mechanically latched cap may seal correctly with the motor OFF, in which case this hypothesis is weakened.
- HC Power(DISABLE) sends zero idle reset, sets CE LOW and all S LOW, and switches VP/VL enables OFF. There is no color-specific10-minute idle action in this path. GPIO level/rail leakage must still be measured physically before excluding unintended idle heating.
- Mapper channels use paired D bits; logical CMY correspond to D1/D2, D3/D4, D5/D6. Live color data, actual magenta D3/D4 signals, S/A timing and loaded VP can be compared before/after idle if cap-state evidence does not identify the cause.
- Current preparation performs mechanical head-OFF wiping and loading; there is no separately controlled suction/air purge or maintenance ink ejection. Sample firmware may have different recovery operations, which requires observation rather than assuming every startup move is a wipe.

## Root fixes and next evidence

1. Prioritize sustained cap sealing: alignment across all color rows, cap compliance/cleanliness, sufficient reproducible pressure, vent/valve state where fitted, and mechanical latching or an appropriate holding strategy if the mechanism requires it. Motor hold should not be enabled blindly: compare sample behavior and check current, temperature and shared power first.
2. Verify nominal drive rather than increase energy indiscriminately: same image/color/nozzle data, magenta D3/D4 electrical levels and relative D/S/A timing, loaded VP, and proper zero/off state during idle. Restore a mismatch to the documented/sample condition, not an arbitrary stronger pulse.
3. Inspect ink/supply conditions if sealing and drive match: magenta wetting/recovery, air/negative pressure and vent behavior. A dose of water improving output is not proof of the failing component.
4. Add measured, bounded maintenance only after waste collection and valid firing location are identified; assess idle duration and required recovery dose. Respect the user's non-paper no-fire requirement. Regular maintenance complements sealing; it must not mask a cap that cannot retain humidity through shutdown.

First focused comparison: record sample U4 EN, SLEEP and RESET levels after it parks (immediately and while idle), then compare with our known release policy. This addresses a concrete software/mechanism difference without re-tuning travel, speed, firing energy and ink simultaneously. If both release their motors, focus next on physical cap sealing or magenta electrical/supply behavior instead of assuming motor holding is needed. Reuse the same image and controlled wipe/water history in later10-minute comparisons.
