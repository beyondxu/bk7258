# V48 pad-position correction and separate CP fault investigation

The reported short backward motion has an explicit software cause: after
GPIO24 LOW, printer_waste_position commanded374 LEFT steps (~2mm) to inset
PRINT_RIGHT. The user now observes the optical endpoint itself aligning over
the sponge and that this retreat displaces the head off it. This does not
establish that mechanical rebound never occurs; it removes the known command.

## Position change

Stationary waste-pad firing now holds at OC3_RIGHT, without retracting. The
TIMER4 ownership is released, but the motor remains enabled at its held phase.
After20ms settle, GPIO24 must remain LOW before issuing the station-ready flag.
The right offset remains374 in the motion coordinate state. This is essential:
the subsequent park must return11221 steps from the optical endpoint, instead
of10847 from the former inset position. Total return from the same optical
reference to the wiper is therefore unchanged; the earlier374-step retreat
has simply moved into the eventual full return, after firing.

Before SYNC, verify the optical reference again after SPI upload/ARM. If it has
released, cancel without starting the row. During stationary playback check it
every wait iteration and abort if it releases. CRC, READY, finite count, RTC/
iteration bounds and no-retry behavior remain. Initial LOW/missing/early OC3
still rejects positioning; this revision does not bypass that protection.

Image stroke/endpoints, paper feed, drive profile, dose, S pulse widths and
HC32 firmware are unchanged. Keep HC32 at214560d. BK identifies itself as
APP_SERVICE_CMY_PAD_HOLD_OC3_V48_requires_HC_prespray_pad16. Look for:
WASTE_POSITION endpoint=OC3_RIGHT retract_steps=0 settle_ms=20 gpio24_LOW.
WASTE_POSITION_LOST identifies reference loss before/during firing.

Tests use the actual timer/cache/SPI path to verify zero LEFT pulses before
firing, stationary firing at the optical coordinate, normal/prespray/normal
profile switching, exact11221-step final park,20ms settle loss, loss after ARM
before SYNC and loss during playback. SDK objects compile; host tests do not
verify physical head-to-sponge alignment or retention force.

## CP crash: not fixed by this change

The supplied dump reports CP crash, rr=0x12. SDK RESET_SOURCE_MPU_FAULT is0x12;
0x11 is HARD_FAULT. CFSR0x82 has DACCVIOL and MMARVALID, with MMFAR0: the
reported data fault address is0x00000000. Thread is CP main, PC0x0208f8bc,
LR0x0208f8af. Secondary-crash reports can arise while handling another fault;
the dump does not establish the original faulting function or hardware cause.

Repeated ENV/EasyFlash startup messages show reboots, not merely a blocked
print worker. The uncali sdmadc warning alone is not the reported MemFault.
No matching CP ELF/map is available in this workspace. Address translation
must use the CP app.elf from the exact flashed BK firmware package; a HC32 ELF,
a different AP ELF, or a new rebuilt binary with a different layout cannot
reliably decode it. Keep the original package and full pre-crash log.

With the matching CP ELF and GNU ARM tools:

```sh
arm-none-eabi-addr2line -e path/to/CP/app.elf -a -f -C 0x0208f8bc 0x0208f8ae 0x0208b50e 0x0208b58a 0x0208df96 0x020816e4 0x02097f24 0x02086388 0x020863ce 0x02085ffc 0x02061148 0x0206120e 0x0209b8c2 0x02061238 0x0208f5bc 0x0208dff2 0x020938c0
```

Return addresses have their Thumb bit cleared; source lines may point after
the calling instruction. Inspect disassembly around the primary PC to identify
the actual invalid operand. Do not claim this position change fixes CP resets.

Restore point for BK: backup/before-pad-backoff-fix-20261123 (9957aa8).
