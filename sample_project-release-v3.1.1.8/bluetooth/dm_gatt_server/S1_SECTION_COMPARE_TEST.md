# S1 against the fixed S2 baseline

This BK-only diagnostic retains the 12-line D1..D6/D6..D1 inspection engine,
startup link guards, first-failure lock diagnostics and corrected APP fault
reporting. HC32 firmware and head electrical settings do not change.

PRINTER_FIXED_S1_COMPARE defaults to 1 in printer_diagnostics.h for this branch.
The existing recipe/check-engine filenames retain S2 names to keep one shared
implementation. Host legacy harnesses default to S2; the dedicated S1 harness
explicitly selects S1. Setting the feature to zero restores S2 golden frames.

## Controlled change

Only the section slot changes from zero-based 1 (S2) to 0 (S1). The eight
one-based address indices, six D masks, X offsets, four selected nozzles per D,
711 commands per nozzle per line, two appearances per D, motor motion and
head ON/OFF sequence are unchanged. Total dose is 1422 commands per selected
nozzle per job, with no priming or retries. This selects different physical
nozzles; improvement alone cannot distinguish a nozzle subset issue from an
S2-specific electrical issue. The S2 physical-row labels do not apply to S1.

## Expected result

One valid APP native-F2 job triggers 12 scans; APP image pixels are ignored:
D1 C, D2 C, D3 M, D4 M, D5 Y, D6 Y, D6 Y, D5 Y, D4 M, D3 M, D2 C, D1 C.
Startup reports APP_FIXED_S1_COMPARE_V1. DOMAIN_CHECK_BEGIN reports section=S1.
HC32 RX_AUDIT: bytes=1416000 crc32=FE431638 A=004A2912 S=01 D=3F;
dots_D1_D6=5688,5688,5688,5688,5688,5688. The former S2 CRC was A13F47F7.
Transport success and returned fire calls do not establish actual ink output.

## First paper trial

Update/flash BK only, retain the present HC32 sampling configuration and supply.
Do not wipe or remove the head. Start both controllers normally and save their
startup logs. Send one job immediately; no deliberate ten-minute idle is needed
for this first area comparison. Photograph the entire page including blank
spaces, mark the leading/feed direction, and save complete BK/HC32 logs. Do not
keep resending if there is no ink. A BOOT_LINK_UNAVAILABLE block must be diagnosed
as a startup link problem, not interpreted as an S1 print result.

The restart required for flashing is an unavoidable difference from the preceding
S2 trial. Interpret a single result as evidence for follow-up, not proof of cause.

## Validation

Independent golden S1 frames assert each nonzero slot is S1, with unchanged
address/D/X support and per-nozzle counts. The actual 12-line controller sends
exact expected bytes, CRCs, and line order under ASan/UBSan, and compiles for ARM.
Fault injection retains no-motion guards, first-error provenance and cleanup.
Legacy S2 domain/color tests pass. BK SDK object compilation covers the worker,
protocol and startup units; full firmware build/flashing and paper verification
are performed on the target setup.
