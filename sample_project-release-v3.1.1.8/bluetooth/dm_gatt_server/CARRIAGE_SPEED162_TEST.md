> Superseded by [V29 paired BK/HC speed trial](CARRIAGE_SPEED182_TEST.md).

# V28: another8.1% carriage cruise increase

Requested another speed trial with the same gear train, without changing microstepping, REF or travel geometry.

- Cruise STEP80us ->74us:8.1% faster than V27, approximately62% faster than the original120us cruise.
- HC column657us ->607us, rounded from985*74/120. This is above the existing protocol minimum600us. It approaches the configured protocol bound; a substantially faster synchronized trial requires reviewing HC actual column execution time, not merely lowering the validation bound.
- Horizontal pitch differs from the120/985 baseline by -0.069% (~0.034mm across50mm). Mapper pitch follows4386 hundredths of a micrometre. Image dimensions remain approximately the same; this does not represent exact integer-clock equivalence.
- Image lead401 steps, ramp351 steps;50 steps of cruise clearance remain. Head-OFF ramps400 steps and start period1000us unchanged. Head-OFF carriage cruise also uses74us.
- Print10473 / service11221, endpoint insets374, six702-step paper-loading increments, electrical firing pulses and microstepping unchanged. Existing HC firmware accepts the per-line607us period from BK, without a firmware update.

Validation: actual cached transport/timer simulations test all1180 columns within cruise in both directions, optical stop/reverse, loading increments, blank travel and final park. Pitch rounding and protocol minimum checks pass. Page-controller tests and six SDK ARM object compilations pass. Hardware noise, smoothness, missed steps and actual ink output remain for the same-image comparison.
