---
layout: post
title: Netcode overview
parent: Articles
permalink: articles/netcode-overview
author: sprunk
---

Here's an overview of some aspects of Recoil's so-called netcode. The **tl;dr is that it uses lockstep simulation which is essentially the same as basically every classic RTS from the 90s and 00s**. Read on for a bit more detailed explanation.

### What gets sent?

**The Recoil network protocol does not involve sending parts of the simulation state**, such as unit positions or health. The **only thing sent over is player inputs**, such as a "move unit X to position Y" order.
How does each client know the game state, then?
The **inputs are sufficient to simulate the gamestate**: if unit X is ordered to move to position Y then its position will change according to its speed (speed being defined in a local file, so does not need to be sent),
maybe it then finds enemy unit Z, which it can shoot according to its range (also defined in a local file) and then that unit's health will decrease by some number - again the calculations will look the same on each client.
Also, keep in mind the **gamestate can easily get magnitudes larger than the size of the inputs** - for example the two orders "build unit, repeat" on a factory will produce a quickly growing gamestate.

### Ping and the packet queue
Of course, packets do not arrive at the server immediately, your ping is involved.
But that is not all: **to guard against network instability**, they are not broadcast by the server to other clients immediately. Instead, **packets are queued for a bit later in a buffer**.
By default this is about 3-6 simulation frames (100-200ms) on top of your "normal" ping, though **games can adjust it and the engine also adjusts it on its own if a match is "laggy"**.
In interfaces such as the `/info` playerlist and `Spring.GetPlayerInfo`, the **ping is in terms of sim frames after taking the queue into account**.

### Sync
What makes the "simulate inputs" approach work is that **the engine takes utmost care to keep calculations identical** on each client.
This is not trivial because you still have to work with **things that naturally differ on each client**, such as mouse position or which units are selected - this is **called the unsynced state**.
On top of that, there can be **hardware differences** that have to be worked around to get identical results - the huge effort involved is one of the reasons why **Recoil is not available outside x86-64**.
Finally, **desync can cascade in a sort of butterfly effect**.
For example, maybe a unit deals a bit less damage than it should, which makes its target survive an otherwise lethal hit;
that unit then bumps into a constructor, slightly delaying the construction of a resource collector.
A difference in stored resources then stems from an incorrect damage calculation minutes earlier.
The **huge effort of hunting down sources of desync** is one of the biggest drawbacks of the architecture.

### Replays and saves
A **replay is just a replication of the network queue from the live game**.
Instead of receiving packets from the server, you read them from your local replay file. Otherwise **everything proceeds largely the same as a live game**.

A save is an **imperfect snapshot** of the gamestate.
While doable, a **perfect replication is difficult and not currently implemented**.
Running identical inputs on a save will proceed differently compared to running them on the original state from which the save was produced.
This means that **using the save mechanism is not currently possible to get mid-game joins or skipping backwards in replays** since it would desync, although could happen if somebody contributes the effort.

### Tradeoffs involved
The **benefits** of this setup are:
* extremely small bandwidth use (remember Recoil has its roots in Total Annihilation back from 1997)
* small replay size
* you cannot easily cheat by modifying your own gamestate (cheat engine style), since there are no inputs that will replicate that on the server.
* the server doesn't need to simulate a game, since clients already do. This makes hosting much easier. See [the dedicated server article]({{ site.baseurl }}{% link guides/headless-and-dedi.markdown %}).
* for development: easy to reproduce bugs - running the same inputs will result in the same simulation, containing the bug.

But there are also **drawbacks**:
* you cannot jump to any specific time, either in a replay or e.g. to rejoin an ongoing game at its current state.
The gamestate at any simulation frame has to be simulated from the previous frame, and that frame has to be simulated from the one before it, beginning all the way back at game start.
* you also have to simulate the whole game state including enemy units, so maphack is possible.
* making sure desync doesn't happen takes a huge effort. 
