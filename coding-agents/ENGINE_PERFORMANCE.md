# Engine performance

Recoil is an RTS engine built for large-scale games — designed to handle thousands of units at once.

## Scale target

- **Target:** ~10k concurrent units, *including buildings*.
- Mobile units tend to be ~40% of that late-game total.
- Largest seen in a real game: ~17.7k units. That's a data point, not a design target.

## Sim, draw, and update frames

The main loop is `Update → Draw`, repeating — see the diagram and table below for the per-phase breakdown. Each iteration first drains any queued sim-frame packets (0..N per iteration), then renders one draw frame. `CGame::Update` dispatches `SimFrame()` calls as `NETMSG_NEWFRAME` packets arrive; `CGame::Draw` runs the unsynced update phase and then renders. The sim burst is capped at ~500 ms (`minDrawFPS`) so draw always gets to run, and it's all one thread — sim and rendering are **not concurrent**; parallelism only happens *inside* a phase.

Conversely, if no sim frames are in the queue the main loop runs `Draw`/`UpdateUnsynced` as fast as possible — many draw iterations can pass between successive sim frames, with visuals interpolating smoothly in between via `globalRendering->timeOffset`.

```
main-loop iteration  (repeats as fast as possible)
├── CGame::Update            (mostly synced)
│   └── SimFrame × 0..N      ← processes queued sim frames capped at
|                              ~500ms per iteration
└── CGame::Draw              (unsynced)
    ├── UpdateUnsynced       ← unsynced update phase
    └── render world + screen   ← Draw::World + Draw::Screen
```

| Phase | Rate | Synced? | Responsibility |
|---|---|---|---|
| **Sim frame** — `CGame::SimFrame` | fixed 30 Hz (`GAME_SPEED`) | mostly yes | advance deterministic state: units, pathing, projectiles, line-of-sight, scripts, Lua `GameFrame` |
| **Draw frame** — `CGame::Draw` | variable | no | update phase (see below) + render world/screen |
| **Update phase** — `CGame::UpdateUnsynced` *(inside draw frame)* | per draw frame | no | timings, interpolation, camera, GUI, sound, world-drawer prep |

### Profiler buckets

The engine `CTimeProfiler` (and the `benchmark` tool) report three peer buckets: `Sim` (the whole synced step), `Update` (`CGame::UpdateUnsynced`), and `Draw` (rendering only, *excluding* the Update that runs first).

`Sim` is **"mostly" synced**: it also bills unsynced work that runs inline during `SimFrame`.
- **Explicit Lua callins** — `GameFrame`/`GameFramePost` run near the start of each sim frame.
- **Event-driven Lua callins** — unsynced widgets can subscribe to synced game events, so their handlers run inline as those events fire during the frame.
- **C++-only unsynced sections** — e.g. the MT projectile visual pass (`Sim::Projectiles::UpdateUnsyncedMT`) and ghosted-building updates (`CUnitDrawer::UpdateGhostedBuildings`).

### Scheduling and CPU budget

- Sim has a target rate set by the server; draw is as fast as the hardware allows. The sim target is `30 Hz × speedFactor`; at a speed factor of 1x, in-game time tracks real-world time 1:1, and at 2x speed the server fires twice as many sim frames per real-world second so the world evolves twice as fast.
- **Zero, one, or many** sim frames per draw frame — if the client falls behind, pending sim frames burst in the next iteration to catch up.
- Visuals interpolate between sim frames, so draw rate can exceed sim rate without stutter.
- Sim time is carefully budgeted and scheduled against draw frames (because they run serially) so there's always a minimum fps for the player

## Multi-threading

The engine runs one **main thread** plus a pool of **worker threads**, all pinned to distinct cores. We typically aim for 6-8 worker threads. The main thread drives the sim/draw loop; workers pick up parallel work dispatched from the main thread (via `for_mt` and friends in `rts/System/Threading/ThreadPool.h`). The main thread also participates in draining the task queue while it waits.

Most parallel work in the engine is **homogeneous** — the same operation applied over many items (unit updates, projectile steps, etc.) via `for_mt`. Keeping parallel work homogeneous is a deliberate discipline: it makes determinism easier to reason about and keeps sim output independent of how work happens to land across threads.

**QTPFS is the one heterogeneous exception.** The quad-tree pathfinder maintains its own per-worker search state (`SearchThreadData`, `SparseData`) independent of engine sim state, which lets it safely run path searches on the worker pool *in the background* via `for_mt_background`. Background tasks yield to higher-priority work by rescheduling themselves when other jobs arrive, so QTPFS soaks up idle worker capacity without preempting foreground parallelism.
