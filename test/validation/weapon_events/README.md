# Weapon event validation

Build `engine-headless` and `basecontent`, then run:

```sh
python3 test/validation/weapon_events/run.py \
  build-amd64-linux/spring-headless /path/to/quicksilver_remake_1.24.sd7
```

For another map, pass its archive and `--map-name 'Map Name'`. The runner creates
an isolated temporary data directory and prints its location, retaining logs for
inspection. It uses the base content beside the supplied engine and a small test
game; it does not need BAR or change an installed game. An engine without the new
API must fail with `missing shot watch API`.

The synced gadget exercises the real weapon update and base gadget-handler paths:

- Three burst shots with four projectiles each: three fired events, twelve
  projectile events, one start and one end; the firing weapon is in slot 2.
- One-shot firing with cloak intent disabled, and firing from cloak.
- A dropped target before the last burst shot: two fired events and one end.
- A script-blocked initial shot: no start, fired, or end event.
- Independent shot/burst watch flags, defaults, and unchanged legacy watch accessors.
- A zero-projectile shot: one fired event and no projectile or script Shot calls.
- A continuous beam: one fired event per engine firing step.
- Callback identity and ordering: projectiles precede the fired event, which
  precedes burst end.

For manual verification, run the same fixture with a newly built graphical
`spring` executable and inspect the `WEAPON_EVENTS` lines in `infolog.txt`.
`WEAPON_EVENTS PASS` and a successful runner exit are required. This is automated
simulation coverage; it does not claim a human gameplay check or a BAR cloak fix.
