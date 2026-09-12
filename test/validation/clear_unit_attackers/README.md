# ClearUnitAttackers validation

Build `engine-headless` and `basecontent`, then run:

```sh
python3 test/validation/clear_unit_attackers/run.py \
  build-amd64-linux/spring-headless /path/to/quicksilver_remake_1.24.sd7
```

The runner creates an isolated temporary directory and retains the logs there.
It uses a minimal test game, the supplied development map, and the engine's base
content. It does not use or modify an installed game. Another map can be supplied
with `--map-name 'Map Name'`.

The fixture checks:

- Full-control LuaRules and `UnitScript.CallAsUnit` access; rejection without
  control, with control of the attacker team, and with control of the target team.
  Denied calls must leave targets and queues unchanged. Unsynced Lua must not
  expose the callout.
- Active A → B → C attacks for stationary, mobile, and strafing-aircraft command
  AIs, plus a duplicate queued attack on A. B must start after clearing A and
  remain active when A is destroyed.
- Queued-only ATTACK, unit-targeted FIGHT and MANUALFIRE orders; unrelated command
  tags/order and ground attacks must be preserved.
- Independent per-weapon targets, weapon-only targets, automatic targets, and
  an attacker whose only order is removed. Other weapon targets must survive.
- Invalid unit IDs, repeated calls, unchanged neutrality, later automatic
  reacquisition, and subsequent explicit attack orders.

Targets are kept alive by blocking shots. MoveCtrl fixes positions except for the
fighter, whose command AI requires its normal movement controller. This does not
test projectile cancellation (the API deliberately leaves existing projectiles
alone) or assess flight paths.
Success requires `CLEAR_ATTACKERS PASS`, `CLEAR_ATTACKERS UNSYNCED PASS`, and a
successful runner exit. An engine without the callout must fail with
`missing ClearUnitAttackers API`.

For manual review, run the same runner with a graphical `spring` build and inspect
the printed checks in the retained `console.log`. For a gameplay check, add the
callout to an isolated game's synced gadget, queue A → B → C, call it on A, and
verify that B is attacked without changing A's neutral state. Do not install the
fixture into a normal game directory.

OpenAI Codex assisted the implementation, test fixture, and documentation.
Automated simulation checks do not constitute human gameplay verification.
