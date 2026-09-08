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

Expected: `PASS blockers=56`. The test controls visibility, reloads and collision
volumes, then checks unit and ground attacks for beam, cannon, straight missile,
curved missile and starburst weapons. Cases cover clear, friendly, neutral,
feature, terrain and simultaneous terrain/friendly obstruction. Starburst's
native test checks its initial vertical cone, so its terrain cases are omitted.

Each callback compares unfiltered, all-categories, friendly-only and terrain-only
queries and verifies the weapon's stored flags and later default query do not
change. It also rejects invalid weapon indices, queries for another unit and
queries outside the callback. The runner rejects missing completion, failed
assertions and callback errors.

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
