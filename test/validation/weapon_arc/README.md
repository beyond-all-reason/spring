# Turret arc regression (#3324)

Requires a BAR checkout and the map archive for Quicksilver Remake 1.24.
The runner creates an isolated temporary data directory and a small mutator;
it does not modify the game checkout or an installed game.

```sh
python3 test/validation/weapon_arc/run.py \
  --engine /path/to/spring-headless \
  --game /path/to/Beyond-All-Reason \
  --map /path/to/quicksilver_remake_1.24.sd7
```

The placement and attack order come from the Wolverine reproducer attached to
#3324. The unit approaches a ground target and stops on a slope. With the old
target-direction arc check, the engine accepts the target but the COB script
rejects the ballistic heading, and no projectile is fired in 2100 frames.
The corrected launch-direction check must let the unit realign and fire by
frame 900, then continue firing through frame 2100.
Use `--expect fail` when running the unmodified engine as a control.

The log includes hull heading and projectile count at frames 480, 900 and 2100.
The runner requires a clean engine exit and the expected final result; logs
remain in the reported temporary directory. For visual inspection, pass the
graphical `spring` executable instead of `spring-headless` and observe the hull
and turret realign before firing.
