---
layout: post
title: Team terminology
parent: Articles
permalink: articles/team-terminology
author: sprunk
---

## Overview

Often players want to play as a team, or conduct diplomacy with each other.
This article aims to explain Recoil's somewhat confusing terminology behind the various team-related entities and how they relate to each other.

### Allyteam vs team

What most people would naturally call a team is called an *"allyteam"* in Recoil terminology.
A *"team"* is a single element of an *"allyteam"*. *Teams* are permanently bound to an *allyteam*.

For example, NATO would be an *allyteam* while USA and UK would be *teams*, because they are separate entities but are in the same alliance, sharing goals and intel.
For modders coming from Supreme Commander, the *team* is what SupCom calls an "army".

*Teams* each have their own:
 * units
 * resources
 * colour
 * starting point
 * faction (aka side)
 * any other custom extra traits you add.

*Teams* in the same *allyteam* always share some traits (meaning that, these operate on the level of *allyteams*):
 * visibility (this includes sight, other sensors such as radar, but also reading of various unit traits that aren't readable by enemies even in sight, for example mana if a game wants to implement that)
 * diplomacy towards other *allyteams* (in particular, all *teams* in an *allyteam* are never hostile to each other)
 * victory goals

### Team vs player

A *team* is generally controlled by a *player* or an AI.
An AI is not considered a *player* and the interfaces for dealing with AIs are separate to those for dealing with players,
but generally they fill the same role of controlling a *team*.

Control of a *team* is not exclusive.
In particular, a *team* can be controlled by two or more *players* (or AIs, or a mix thereof) simultaneously (think SC2 Archon Mode).
This is sometimes referred to as "comsharing" in the community for historical reasons.
In that case all of them can exercise control over the team equally.

Control of a *team* is also not permanent.
*Players* can start controlling a different team (losing control of the previous *team*).
In particular *players* can also control no team at all, in which case they're just spectators.
It is up to the game to let *players* change their *team* to a different one (they cannot do so at will), though they can always become a spectator.

To go with the analogy above, if the US Army is a *team* then Patton and Eisenhower are *players*: they both have control over the army (simultaneously with each other), and the army itself is generally unaffected by personal changes in the command staff: it is not bound to its generals, but the generals are bound to the army. 

A *team* can also have no controllers.
This usually happens when somebody disconnects, but you can have teams that are uncontrolled by design (for example to have a perspective change in a singleplayer mission, or to have rescuable units).

### Regular player vs spectator

Spectators are *players* not controlling any *team*.
They have two modes of spectating, one is a full-view mode where they can see everything, and the other is to spectate as if they were on a particular *team* (meaning their UI will display that team's resources, they will only see what that team sees, etc).
Spectators can change what team they are spectating, and to and from the full-view mode, at will.
The engine also has an option to allow new players to join mid-game (as opposed to regular players reconnecting); such added people always end up as spectators (in particular, this means they will have seen the full state of the game while simulating).

### Player vs AI

An AI fulfils a similar role to a *player*, being there to control a *team*.
One difference is that an AI is permanently bound to a *team* and cannot spectate.
The API to deal with AIs is also separate to *players* (so, for example, the function to get all *players* will not return them, and a *team* may look uncontrolled if care is not taken to handle both of its *player* and AI controllers). 
There are two main types of AI: Lua AI and Skirmish AI.

### Lua AI vs Skirmish AI

A "skirmish" AI is hosted by one of the players and generally acts very similar to a *player*.
It can read the game state via AI interface and works by giving units commands.
Strictly speaking, it is their hosting player relaying commands - this means that this type of AI is subject to lag and will drop if the hosting player quits.
On the other hand, only the host player is taking on the burden of simulating the AI.
There are currently Skirmish AI bindings for C and Java (though distributing the Java runtime environment for a Java skirmish AI is up to the game).
A game does not need explicit support for this kind of AI (meaning for example, somebody can homebrew one), though it will likely want to handle distribution and infrastructure issues (for example to block homebrew AI).

A Lua AI generally has two components: a piece of game mechanics, and the AI instance itself which is just a handle to tell game mechanics which teams are legal to control. 
Since game mechanics have full control over the game state, this type of AI can do things like spawn units on its own (for a sort of PvE experience) .
It can also control teams that aren't explicitly marked for control by the LuaAI handle (for example a LuaAI can be made to automatically control AFK players' teams, or Gaia units - see below).
This type of AI is written in Lua (like all game code), has to be included in the game itself, and the code runs for every player in the game.

### Gaia

Gaia is a special *team* that is always present, uncontrolled, and is always in its own *allyteam*.
It is generally meant for ownership of map features such as trees and rocks, but it also can have resources and own units.
This can be used for example for PvE enemies - note that game mechanics still have control an uncontrolled *team* so it is not necessary to have an explicit AI for it.
At the moment there is only one Gaia.
