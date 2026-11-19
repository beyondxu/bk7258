> V25 shortens both single strokes by1mm: see [current travel and dry-ink investigation](TRAVEL_MINUS1MM_DRY_INK_TEST.md).

# V24 unified carriage travel

User-approved geometry: print stroke is the previous11034-pulse stroke minus374 pulses (~2mm total); boot and paper-loading preparation share a stroke748 pulses (~4mm) longer than printing. Final parking uses the same service endpoint, without an additional retreat beyond it.

| Stage | Nominal pulses |
|---|---:|
| Print scan | 10660 |
| Boot seek cap / return | 11408 |
| Concurrent paper-loading seek cap / return | 11408 |
| OC3-to-final-wiper return | 11408 |
| Inset from each service endpoint to the print endpoint | 374 (~2mm) |

GPIO24 LOW still stops RIGHT motion immediately. Search caps do not override the switch. Before a LEFT image scan from the optical service endpoint, move LEFT374 pulses with the head OFF; from the wiper move RIGHT374 pulses before a RIGHT image scan. Both image directions then scan10660 pulses. Final parking from PRINT_LEFT is374 pulses LEFT; from PRINT_RIGHT is11034 pulses LEFT; from OC3 is11408 pulses LEFT. This prevents conflating the optical RIGHT endpoint with the inset print endpoint.

The1180 image columns,985us column period,120us cruise STEP period and image dimensions are unchanged. Wiping remains two20mm out/back rounds. Paper loading remains702 steps per leg, six legs (10.8mm total).

## Physical calibration

This unified return is374 pulses (~2mm) longer than V22/V23's calibrated11034-pulse sensor-to-wiper return. Earlier hardware logs showed OC3 after11057 pulses and contact with the wiper-side frame after a11408-pulse return. The user's later unified-distance request reinstates11408 as the common nominal return; software simulation cannot establish mechanical clearance. First observe a single boot cycle and check the wiper-side clearance before printing. GPIO24 protects the sponge side only.

## Validation

- Actual cached transport/timer simulation, including optical stop, both image directions, loading and final parking.
- Unified11408/10660 relationship,374-pulse endpoint insets,1180 image columns and unchanged4212 total paper steps.
- Preparation sequencing and page-controller simulations.
- Six BK SDK ARM objects compiled, including application and motor/timer drivers.
