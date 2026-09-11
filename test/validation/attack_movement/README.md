# Attack movement validation

Use an isolated development data directory, a headless build, BAR with the
`AttackCommandMovement` handler and `unit_attack_movement.lua`, and development
map assets. Never point these tests at a normal installed game directory.
Copy `check.lua` into the test game's `luarules/gadgets/` directory.

## Replay comparison

Check out the BAR revision matching the replay and copy the handler changes and
**the identical policy gadget** from the BAR companion PR into that checkout.
`prepare.py` supports demo format 5 and creates an isolated directory archive,
rewrites the game name/modoptions, accelerates packet delivery and drops old
engine sync-response packets. It hardlinks game files: keep the source checkout
unchanged during all runs. It does not alter the original demo or engine.

```sh
python3 test/validation/attack_movement/prepare.py REPLAY OUTPUT \
  --bar TEST_BAR --assets DEVELOPMENT_DATA --engine ENGINE \
  --mode native --frames 36000
python3 test/validation/attack_movement/run.py ENGINE OUTPUT OUTPUT/input.sdfz
```

Repeat with separate output directories and `--mode lua` and `--mode fallback`.
Compare every `[AttackValidationSnapshot]` payload. Each hashes sorted unit IDs,
positions and current command ID/options/tag every 1,800 frames. Matching
snapshots are a movement regression check, not a complete simulation checksum
or a performance benchmark. A complete run must print `[AttackValidation] PASS`.

## Blocker/filter matrix

Prepare an isolated game directory as above. Install Quicksilver Remake 1.24
in the development maps directory and run `blockers.txt` instead of the demo:

```sh
python3 test/validation/attack_movement/run.py ENGINE OUTPUT \
  test/validation/attack_movement/blockers.txt
```

Expected: `PASS blockers=86`. The test controls visibility, reloads and collision
volumes, then checks unit and ground attacks for beam, cannon, straight missile,
curved missile and starburst weapons. Cases cover clear, friendly, neutral,
feature, terrain and simultaneous terrain/friendly obstruction. Starburst's
native test checks its initial vertical cone, so its terrain cases are omitted.
Additional cases cover static allies and mixed mobile/static allies in both
creation/spatial orders. Each source identity must match the actual blocker;
clear, range, terrain and unchecked rotation must not retain a previous source.

Each callback compares unfiltered, all-categories, friendly-only and terrain-only
queries and verifies the weapon's stored flags and later default query do not
change. It also rejects invalid weapon indices, queries for another unit and
queries outside the callback. The runner rejects missing completion, failed
assertions and callback errors.

Candidate-position checks compare the current position/native heading with the
heading query, test a distant out-of-range position and both pre-aim/muzzle modes,
and verify restoration of position, heading, weapon vectors and default queries.
Narrow blockers provide explicitly blocked-current/clear-candidate cases for
sidestepping. This checks shot geometry, not candidate path reachability.

The query-only `noMobileFriendlies` and `noStaticFriendlies` masks subdivide
allies by UnitDef movement capability. Existing masks/default weapon flags keep
their previous meaning. Scripts can query with just mobile allies remaining,
then separately test buildings/terrain; a first blocker ID alone cannot answer
whether both kinds obstruct the shot.
The matrix also verifies that previously unused high bits in stored avoidance
flags do not opt native/default queries into the new filters. Actual firing
raytraces do not receive the query-only filtering argument.

For interactive review, use the same placements and masks with a graphical
engine. Compare `attackmovementmode=lua` with the unset option and `fallback`.
The equivalent BAR policy deliberately retains the native 90% range behavior;
these tests do not claim that all behavior reports in #3331 are fixed.

## Command lifecycle

Also copy `ended.lua` into the isolated game's gadget directory, then run
`ended.txt` through `run.py`. Expected: `PASS ended=10`. Cases cover completion,
explicit removal, target death, replacement, front insertion, WAIT suspension and inactive queued
removal. Empty queues and WAIT successors must stop when the callback clears the
old goal; MOVE successors must remain queued and continue. The callback checks
the old tag and reason and observes the successor before it executes.

`UnitCommandEnded` supplements `UnitCmdDone`; it does not change its legacy
notification contract or install a default stop policy. `completed` means the
CAI finished the command, not necessarily that its gameplay objective succeeded.
The hook covers shared FinishCommand and explicit queue editing paths; internal
CAI subcommand insertion is not a general suspension/resumption notification.

## Dispatch ownership and invalidation

Install `dispatch_rules.lua` in the isolated game's gadget directory and
`dispatch_gaia.lua` as that game's `LuaGaia/main.lua`. Quicksilver has map-side
LuaGaia entry points; the mod-first loader selects this opt-in fixture. Run
`dispatch.txt` through `run.py`. Expected: `PASS dispatch=6`. The fixture checks
that true stops dispatch across LuaRules/LuaGaia, false/nil reaches Gaia once,
and replacing the command or destroying its target/owner prevents further
dispatch. The candidate and movement APIs reject a context invalidated by Lua.
Remove the fixture from any game intended for actual play.

## Manual review and remaining parity gate

In an isolated graphical game with the companion handler enabled, place a
stationary shooter, an enemy target and an allied building between them. Inspect
the current-position result and two candidate positions with
`TestUnitAttackMovementPosition` inside the movement callback. Confirm the
building's ID and a clear candidate, then repeat with a mobile ally, both
blockers, terrain, ground attacks, and a cannon/curved missile. Querying alone
must not move the shooter or rotate its model. Enable a BAR movement decision
only in a separate policy change, after the identical Lua policy passes parity.

Neither the old replay snapshots nor these focused tests certify 100% identical
gameplay. Acceptance still requires a frozen parent/no-call-in/false-fallback/Lua
comparison per frame, wider replay and policy-branch coverage, and human review.
OpenAI Codex assisted the API implementation, fixtures and documentation;
automated headless checks do not substitute for human gameplay verification.
