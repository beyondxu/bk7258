# Remove the black prefix to check the reported all-blank regression

The user reports no visible ink at any time since flashing 2f40778, including
an immediate repeat. The previous 17ca74d trial had a faint upper yellow and a
strong lower yellow. This temporal association warrants a firmware A/B rollback
before attributing the new all-blank state to head condition or asking for more
reference-machine tests. It does not establish that firmware damaged the head.

Only PRINTER_BLACK_PREFIX changes from 1 to 0. The added CMY prefix build and
scan are compiled out. The original yellow page with S1 rows29..43 remapping
and unchanged S3 control remains, as in the 17ca74d test. No voltage, HC32 timing,
GPIO mapping, feed distance, APP parser, upper-nozzle set or yellow dose changes.
The startup banner returns to APP_YELLOW_S1_REMAP_V1. No BLACK_PREFIX_BEGIN
should occur. Historical prefix sources/tests remain available but inactive.

Flash BK only, start the whole machine from power off, keep the same head,
board and APP image. Do not wipe or swap the cartridge before the first rollback
print. Send the image once and save paper/logs. Do not run an idle study or
repeated purges. This first observation is only whether any output, especially
the previously strong lower yellow, returns after removing the added prefix.
A still-blank page is not proof that the prefix alone caused the failure; cold
restart, hardware and head state remain possible factors.

The yellow-remap controller regression checks actual task1059 mapped data,
original per-nozzle dose and S3 bytes with no prefix. BK SDK source objects
compile; target-side ink recovery still requires this one controlled trial.
