# Backwards compatibility

Recoil isn't 100% beholden to backwards compatibility, but breaking changes are weighed carefully against their benefit. Backwards compatiblity is a constraint, not a veto.

## Why the bar is high

Recoil games aren't short-cycle Unreal/Unity titles — they're lifetime hobby projects. There is no steady stream of new games picking up the latest engine; the games we have are the games we have. They fall into two camps, and neither absorbs churn well:

- **Mature games** need stability above all else.
- **Games still in active development** have the flexibility, but rarely the volunteer bandwidth to chase significant engine breakage.

## How to weigh a change

- Quantify the benefit (perf, correctness, maintainability) concretely, not in the abstract.
- Identify which games or content would break, and how mechanical the fix is on their side. "Rename a call site" is very different from "rearchitect your gadget."
- Prefer changes whose blast radius is contained or whose adaptation is mechanical. Avoid changes that force games to rethink core logic with no real mitigation path.

## Precedents

- **Multi-threaded unit movement & collision** — landed with a large perf win and effectively no game-side impact (ignoring incidentally-fixed bugs). This is the shape of change to look for.
- **Multi-threading `Unit::Update`, `Unit::SlowUpdate`, or projectiles** — don't. The impact on games would be huge and there isn't much that can be done to mitigate it. Not a path worth proposing.

## The upshot

Backwards compatiblity constraints don't close the door on performance work — they just point it at the areas where the blast radius is small. Plenty of wins are still on the table; pick the ones games don't have to pay for.
