---
layout: post
title: Technicalities of starting a match
parent: Articles
permalink: articles/technicalities-of-starting-a-match
author: sprunk
---

What does starting a game look like, from a technical point of view? What are the responsibilities of each component?

### Engine

The only thing that the engine needs to initiate a game is a _start script_: plaintext data containing the setup such as game, map, players, modoptions, etc. Alternatively, the engine can use match metadata (in particular, host IP) to connect to a match hosted elsewhere. Then it receives the startscript directly from the host, after connection.

* in-engine Lua code can start a game via `Spring.Restart(string commandLineArgs, string startScript)`. This is available from LuaMenu (e.g. Chobby) but also regular ingame Lua instances.
* external lobbies can run the engine and pass the startscript/metadata as standard input or command line args.
* replays and save files generally work equivalently to a startscript (and contain the startscript for their game).
* **practical tip**: during development it's convenient to have a `startscript.txt` file and drag-n-drop it to `spring.exe` (or equivalent). The engine leaves a `_script.txt` file containing the last used startscript so you can produce an intricate setup via a graphical lobby, or by watching a replay, and then reuse it.
* **practical tip**: replays are associated with a specific engine version, so if you want to support watching old replays you'll need to handle it externally (it can be a simple script communicating with LuaMenu via a socket though, not necessarily a "full" lobby).

Every other component's role is merely to facilitate the creation of startscripts and exchange of metadata.

### Lobby client

Either in-engine LuaMenu such as Chobby, or external programs.

* for singleplayer, or multiplayer games where the player is the host, lobbies are responsible for generating the startscript based on game setup.
* sometimes you already have a startscript somehow (e.g. maybe it comes from a mission archive, or you're just running a replay) and then the lobby is just responsible for passing it to the engine.
* for multiplayer where the player is not a host, lobby clients handle receiving match metadata from the host, usually via a lobby server.

### Autohost

As far as the process of starting a game, autohost is just a stub lobby client.
* it is practically always used as the host of a room on a lobbyserver, and thus needs to generate the startscript based on game setup.
* **practical tip**: some lobby servers claim that a player "is the host" of a room where actually he is just the boss of a room where the actual host is a bot/autohost. In this case the autohost is responsible for generating the startscript. Don't confuse those.

### Lobbyserver

The lobbyserver is only responsible for transmitting metadata from the host to the other players so they can connect, but you can use any other alternative. For example one could use the Steam p2p direct connection for transmitting metadata to play co-op with a friend.

### Startscript format

See [here](https://github.com/beyond-all-reason/spring/blob/BAR105/doc/StartScriptFormat.txt).
