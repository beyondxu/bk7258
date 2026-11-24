> V53 long/unknown-idle dose is1600,40 cache rows. See FAST_PRESPRAY_300US.md.

> Current V52 changes only automatic prespray to300us/5ms; BOTH MCUs must
> be updated. See FAST_PRESPRAY_300US.md. Earlier period/pause details below
> describe previous comparisons.

> V51 extends only the long/unknown-idle dose:400 → 800 requests/nozzle,
> 20 cache rows instead of10. Short/medium doses remain10/100.
> Batch OFF pauses remain20ms;32-NOP HC profile remains unchanged.
> Nominal playback plus pauses is16.26s (20*1180*673us +19*20ms),
> excluding transfers and carriage movements. This is requested firing,
> not confirmed droplets or a measured thermal guarantee.
> Only BK needs updating, with the existing HC32 pad32 firmware972fa1d.
> Banner: APP_SERVICE_CMY_PAD_LONG_PRESPRAY_V51_requires_HC_prespray_pad32.
> Restore tag: backup/before-prespray-800-20261124 (BK216feed).
> Previous V49/V50 comparison details below are historical for dose/time.

> V50 uses the new HC32 prespray32-NOP profile (previously16).
> Update HC32 first, then BK. Dose, pattern and waits below are unchanged.
> See PRESPRAY_FIRE_PROFILE_TEST.md for the theoretical80ns increment.

# Compact prespray V49

BK branch: `codex/printer-startup-load-prep`. Requires the existing HC32
prespray profile with32-NOP padding; HC32 revision214560d is the old16-NOP version.

| Idle | Requests/nozzle before → now | Cache rows | Explicit OFF pauses before → now |
| --- | ---: | ---: | ---: |
| <3 minutes | 5 → 10 | 1 | 0 |
| 3..<15 minutes | 50 → 100 | 3 | 200 → 40 ms |
| >=15 minutes / unknown boot | 200 → 400 | 10 | 900 → 180 ms |

Each nonempty transport column selects one Domain and two Sections:
S1/S2 or S3/S4. Each section pair's25 addresses are split24+1, giving24
groups/round and40 rounds/cache. Previously48 groups/round and20 rounds/cache.
All600 physical nozzles receive exactly the selected dose. No data byte has
multiple Domain bits. At most2 requests/address (2 selected sections) and48
requests/column, distributed across addresses. The column total is not48
simultaneously fired heaters. Instantaneous load is higher than V48, so stable
VP and ink recovery must be verified on hardware; they are not inferred.

A nozzle still receives at most8 consecutive-column requests,673us apart;
then other groups run. The HC32 normal/prespray tFire profiles, S low width,
25+1 stop, Ax/Dx zero reset and completed-column idle guard are unchanged.
No TSR-based or open-loop preheat is added. Normal24-row S1 image mapping,
size, feed, carriage speed and stroke are unchanged.

Total requests double without adding cache rows, uploads, positioning or return
moves. With1180 columns/cache and673us/column, long-dose playback plus explicit
pauses is nominally about8.12s versus8.84s. Transfer/handshake/position/return
are additional and unchanged; actual total time is unmeasured.20ms OFF pauses
are additional to upload and power-on delays. No promise of overnight recovery.

The RTC policy tracks completed firing requests, not actual droplets. Unknown
boot/RTC rollback uses400. Successful nonblank scans/prespray update the clock.
Boot and pre-page entry points and failure handling remain as before.

OC3_RIGHT positioning retains zero retreat and20ms settle with continuous
active-LOW optical checks during stationary firing. Afterwards power OFF and
park at the wiper. Optical loss, CRC/profile/timeout errors stop and block the
image job; no automatic retry. See PAD_POSITION_AND_CP_FAULT.md.

Validation: independent nozzle CSV checks600 unique routes, exact doses up to40,
paired-section concurrency and8-shot bursts; RTC boundaries/rollback; job doses,
1/3/10 cache rows,20ms OFF pauses and failure cleanup; boot/page and waste-motion
integration; actual SDK ARM object compilation. Host checks do not measure
pulse width, current, voltage, temperature or ink flow.

On hardware, identify `APP_SERVICE_CMY_PAD_STRONG_PRESPRAY_V50` and
`PRESPRAY_BEGIN ... rounds=400 ... one_D_two_S_per_column row_rounds=40 ...
between_rows_ms=20`. Cold boot should end with10 batches, each40 rounds.
Compare the same head's ink recovery and BEGIN→DONE time after overnight idle.

Rollback: `backup/before-compact-prespray-20261124` points to7f5f5a0 (V48).
It preserves the previous200-shot single-Section pattern and100ms pauses.
