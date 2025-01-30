---
layout: post
title: VFS Basics
parent: Articles
permalink: articles/vfs-basics
author: sprunk
---

## VFS basics

### What is loaded?

Recoil Engine **loads content from three main places** into its Virtual Filesystem (VFS): the game, the map, and the user's local files.
The game and the map **are specified by the lobby** when launching an instance of Recoil.
**Game content is primary** and it usually decides whether to even load content from the other two, with some exceptions (for example map layout is always taken from the map archive).

### Archives

Games and maps **typically come as archives** (a single compressed file).
For development purposes, they can also be **regular folders as long as their name ends with ".SDD"**, though this is **not recommended for production** (for performance reasons).
Archives **can specify dependency** upon other archives, so **if you want to build upon an existing game you don't need to copy-paste its whole contents** (and probably shouldn't).
By default, games live in the `./games/` subfolder of your Recoil folder and maps live in `./maps/`, but a **lobby can specify arbitrary archives**.

{: .warning }
> At the moment, **files in dependencies are completely overridden** in the VFS and are not accessible.

The user can also **specify local content folders** which are then loaded from, **if the game allows** that.
By default, the Recoil folder is the read directory.
In this case, there is usually **no archive - loose files are seen by the VFS**.
Of course **an archive can be such loose file** and there are interfaces to load its contents.

### Sync

The **game and map are synced**, meaning their contents can be used as **authoritative data related to the simulation**.
Local files are unsynced, so they **cannot even be accessed from synced contexts**.
For example, the game can define that a tank has 100 health.
The game **can give the map an opportunity to modify this**, so for example the map can say the tank has 200 health instead.
But there is no way for local files to modify that further, unit health is part of the game mechanics simulation so the game cannot defer to local files here even if it wants to.
What this means is that **local files are largely for local content like the UI**, and it is generally **safe to assume the VFS is under a game dev's control even if you don't pay attention**, as far as mechanics are concerned.

### How do I defer to the other content?

VFS interfaces tend to **expect a _mode_ parameter**, which lets you specify **where to look for and in what order**.
The values are strings so **you can combine them** using the `..` operator, and they are read left-to-right.
For example, if you want to include a file from the map but also have a game-side backup, you'd pass `VFS.MAP..VFS.GAME` to `VFS.Include`.
Since `VFS.RAW` was not passed, any existing loose file with the appropriate name among the user's local files is ignored.
See below for a listing of modes accessible to Lua.

By paying attention to the VFS mode, you can **prevent loading unwanted content**.
As mentioned above, requesting unsynced modes in synced contexts is also ignored.

### Loose remarks

There is a fourth place where content is loaded from, the **basecontent** archive.
The engine **always loads** it and it contains various **bare necessities** for a functional game, such as default water texture or the basic wupget frameworks.
Its content is **entirely optional** and can be avoided via the usual VFS mode interface, though even mature games will usually want to make use of its facilities.

The engine **can also write files** in addition to reading them.
Unlike multiple read-folders, there is **only a single write-folder** (defaults to the Recoil folder).
Writing is done by general Lua interfaces such as `io` and `os`, not `VFS`.

A somewhat unorthodox way to pass (synced) content is **via modoptions**.
Modoptions can **contain data** that gameside code can act upon, and if you're brave enough you can even **pass Lua code** as a modoption to be executed.
This is one of the ways to let people **run their local files in a synced way**, by just forwarding them as modoptions.

## VFS mode listing
Here are the fundamental modes:

* `VFS.GAME` - anything included by either a game OR its dependencies, as long as they are not basecontent. There is no way to tell where a file comes from, but dependencies override!
* `VFS.MAP` - the map archive.
* `VFS.BASE` - loaded basecontent archives. Some of those are always implicit dependencies. Any dependency with the appropriate archive type fits though.
* `VFS.MENU` - the loaded menu (lobby) archive, i.e. in practice Chobby.
* `VFS.RAW` - anything not in an archive, i.e. any loose files the client may have in his data folder, or even anywhere on the filesystem. Only unsynced content can access these.

And here are the convenience/legacy ones:

* `VFS.ZIP` = `VFS.GAME .. VFS.MAP .. VFS.BASE`. Synced content can only use subsets of this. Note that this doesn't mean actual `.zip` (aka `.sdz`) archives, `.sdd` and `.sd7` still apply.
* `VFS.ZIP_FIRST` = `VFS.ZIP .. VFS.RAW`.
* `VFS.RAW_FIRST` = `VFS.RAW .. VFS.ZIP`.
* `VFS.MOD` = `VFS.GAME`. This is NOT for mods, they are indistinguishable from the main game from the VFS' point of view! It's just a legacy synonym.
* `VFS.RAW_ONLY` = `VFS.RAW`.
* `VFS.ZIP_ONLY` = `VFS.ZIP`.
