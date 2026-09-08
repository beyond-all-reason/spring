+++
title="First Steps with the Engine"
+++

Recoil Engine is a powerful open-source RTS engine that lets you create large-scale
real-time strategy games using Lua scripting. This guide will help you set up your
development environment and run your first game.

> [!NOTE]
> The engine binary is currently named `spring`. This is a temporary name inherited from
> the original Spring RTS engine and will be renamed to `recoil` in a future release.

## Quick Start

The fastest way to get started with Recoil game development:

1. **Download the engine** from the [Recoil releases page](https://github.com/beyond-all-reason/RecoilEngine/releases)
2. **Download maps** from [Beyond All Reason's map repository](https://www.beyondallreason.info/maps)
3. **Clone a beginner-friendly game** like [RecoilExampleMod](https://github.com/DarkBlueDiamond/RecoilExampleMod) or [VroomRTS](https://github.com/DarkBlueDiamond/VroomRTS)
4. **Place assets in the correct directories** and run the engine

## Where to Put Games and Maps

Recoil Engine looks for games and maps in several locations, checked in order of priority.

### Engine Directory (Recommended for Development)

Place your game and map archives in the corresponding subfolders within the engine directory:

```
recoil-engine/
├── games/          # Your game archives (.sd7, .sdz, .sdd)
├── maps/           # Your map archives
└── base/           # Base content (provided by engine)
```

This is the simplest approach and ideal for development.

### Isolated Mode

For portable installations or self-contained game distributions, create a file called
`isolated.txt` in the same directory as the Recoil executable, or set the 
`SPRING_ISOLATED` environment variable to point to your data directory.

In isolated mode, the engine only uses data from the specified directory:

```
your-game/
├── recoil-executable
├── isolated.txt      # Create this file
├── games/
├── maps/
└── base/
```

You can also enable isolation via command line:

```bash
./spring --isolation
```

To specify a custom isolation directory:

```bash
./spring --isolation --isolation-dir /path/to/game/data
```

### Write Directory

Recoil needs a writable location for saves, replays, logs, screenshots, and other
generated files. By default, the first writable directory in the search order becomes
the write directory.

**Command-line option:**
```bash
./spring --write-dir /path/to/writable/dir
```

**Environment variable:**
```bash
export SPRING_WRITEDIR=/path/to/writable/dir
./spring
```

**Priority order for write directory:**

| Priority | Source | Description |
|----------|--------|-------------|
| 1 | `--write-dir` flag | Command-line override |
| 2 | `SPRING_WRITEDIR` env | Environment variable |
| 3 | Isolation mode dir | When `isolated.txt` or `SPRING_ISOLATED` is set |
| 4 | Portable mode | When `springsettings.cfg` exists next to binary |
| 5 | Home directories | `~/.spring`, `%USERPROFILE%\Documents\Spring`, etc. |

> [!NOTE]
> The engine changes its working directory to the write directory after initialization.
> Relative paths in configuration resolve from this location.

**Write directory vs. isolation mode:**
- `--isolation` limits **where to find** games and maps
- `--write-dir` sets **where to save** generated files
- They can be combined for portable setups

```bash
# Portable setup: read from /media/game, write to /home/user/saves
./spring --isolation --isolation-dir /media/game --write-dir /home/user/saves
```

### User Data Directories (Default for Installed Engines)

When not in isolated or portable mode, Recoil also searches in user-specific locations:

**Linux:**
- `~/.spring/`
- `${XDG_CONFIG_HOME:-~/.config}/spring/`

**Windows:**
- `%USERPROFILE%\Documents\Spring\`
- `%USERPROFILE%\Documents\My Games\Spring\`
- `%ProgramData%\Spring\`

These locations are useful when you want to share content between multiple installed
versions of the engine.

### Path Encoding

> [!NOTE]
> Starting from releases `2026.xx.yy`, Recoil supports Unicode paths (e.g., paths
> containing non-ASCII characters like ä, ö, ü, or CJK characters). Earlier versions
> require strictly ASCII paths and will fail to find games or maps in directories
> containing non-ASCII characters.

## Archive Formats

Recoil supports games and maps in archive formats or as uncompressed directories.

### Compressed Archives

| Format | Extension | Compression |
|--------|-----------|-------------|
| SD7 | `.sd7` | 7-Zip (LZMA) |
| SDZ | `.sdz` | ZIP (Deflate) |

For distribution, compressed archives are preferred for smaller file sizes and faster
downloads. SD7 offers better compression ratios.

### Development Directories

For development, games and maps can be stored as regular directories with a `.sdd`
extension:

```
games/
├── MyGame.sdd/           # Development version
│   ├── ModInfo.lua
│   ├── units/
│   └── ...
└── MyGame-1.0.sd7        # Release version
```

A `.sdd` is **just a normal directory** — there is no special format, archive, or build step.
The only requirement is that the directory name ends with `.sdd`. You can create one with
a simple `mkdir MyGame.sdd` and start adding files. The engine detects the `.sdd` suffix
and reads the directory contents directly, so any changes you make to files inside it take
effect the next time you start a game — no repacking needed.

### Cloning a Game for Development

Clone a beginner-friendly game directly into your `games/` directory with the `.sdd`
extension so the engine picks it up immediately:

**RecoilExampleMod — minimal template:**
```bash
cd recoil-engine/games/
git clone https://github.com/DarkBlueDiamond/RecoilExampleMod.git RecoilExampleMod.sdd
```

**VroomRTS — simple gameplay:**
```bash
cd recoil-engine/games/
git clone https://github.com/DarkBlueDiamond/VroomRTS.git VroomRTS.sdd
```

After cloning, your engine directory will look like this:

```
recoil-engine/
├── spring                  # Engine binary
├── games/
│   ├── RecoilExampleMod.sdd/    # Cloned from GitHub
│   │   ├── ModInfo.lua
│   │   ├── LuaRules/
│   │   ├── LuaUI/
│   │   ├── units/
│   │   └── ...
│   └── VroomRTS.sdd/            # Cloned from GitHub
│       ├── ModInfo.lua
│       ├── LuaRules/
│       ├── LuaUI/
│       ├── units/
│       └── ...
├── maps/                        # Place downloaded maps here
└── base/
```

Now launch the engine and select either game from the menu:

```bash
./spring
```

To update a cloned game to the latest version:

```bash
cd recoil-engine/games/RecoilExampleMod.sdd/
git pull
```

## Content Delivery with Rapid

Recoil includes support for the **Rapid** content delivery system, a legacy mechanism
for incremental downloads of games and maps from remote repositories.

### How Rapid Works

Rapid uses a content-addressed storage system with two special directories:

```
data-directory/
├── rapid/                   # Repository metadata
│   └── repos.springrts.com/ # Per-host metadata
│       └── my-game/
│           └── versions.gz   # Tag-to-package mappings
├── packages/                # .sdp index files (archive manifests)
│   └── abc123def.sdp        # One file per archive version
└── pool/                     # Content files, organized by hash
    └── ab/                   # First 2 hex chars of MD5 hash
        └── c123def...gz      # Gzipped file content
```

**The flow:**
1. `versions.gz` maps a tag (e.g., `my-game:test`) to a package hash
2. The `.sdp` file in `packages/` lists all files in that archive version with their MD5 hashes
3. Each file's content is stored in `pool/` under a path derived from its hash

This enables incremental updates—when a new version releases, only changed files are downloaded. The `GetRapidPackageFromTag()` function resolves tags to package names using the metadata.

### Current Issues

> [!WARNING]
> The primary Rapid repository `repos.springrts.com` is hosted on Spring infrastructure with reliability concerns. The Spring project is largely unmaintained, and the repository may become unavailable.

**Problems with Rapid today:**
- **Infrastructure instability**: Hosted on community servers with uncertain maintenance
- **No CDN**: Single point of failure for downloads
- **Legacy protocol**: Not designed for modern hosting (GitHub releases, etc.)
- **Complex setup**: Requires maintaining Rapid metadata alongside game code

### Recommendations

For new projects, avoid Rapid and use modern distribution:

| Method | Pros | Cons |
|--------|------|------|
| **Self-hosted CDN** | Full control, reliable | Requires infrastructure |
| **GitHub Releases** | Free hosting, versioning | Manual download for users |
| **Direct archives** | Simple, works everywhere | No incremental updates |
| **Game launcher** | Automatic updates | Requires custom tooling |

Many games ship a launcher or use existing lobby clients that handle downloads from modern sources. If you need incremental updates, consider implementing delta patches on top of SD7/SDZ archives rather than using the pool/packages system.

## Getting Maps

Maps are required to play any game. The most up-to-date and curated map repository is
[Beyond All Reason's maps page](https://www.beyondallreason.info/maps).

Alternative sources include:
- Spring's historical map archives
- Community forums and repositories

Download maps in SD7 or SDZ format and place them in your `maps/` directory.

## Learning from Existing Games

The best way to learn Recoil development is to examine and modify existing games.
The Recoil homepage lists [all games using the engine](/) with varying complexity.

### Beginner-Friendly Games

Two games are particularly suited for newcomers:

{{< cards >}}
{{< card title="RecoilExampleMod" subtitle="A basic game template showing the fundamental structure of a Recoil game. Each folder is documented, making it easy to understand how games are organized." link="https://github.com/DarkBlueDiamond/RecoilExampleMod" >}}
{{< card title="VroomRTS" subtitle="A straightforward RTS game with vehicle combat. Good for understanding basic game mechanics without overwhelming complexity." link="https://github.com/DarkBlueDiamond/VroomRTS" >}}
{{< /cards >}}

### Modifying Existing Games

1. Clone a game repository or download a release
2. Place it in your `games/` directory as a `.sdd` folder
3. Launch the engine and select your game
4. Edit Lua files and retest

Changes to Lua code take effect on the next game start. For workflow tips,
see the [Lua Language Server guide](lua-language-server.md) for setting up IDE
autocompletion.

## Next Steps

Once you have the engine running with a game and map:

- Learn about [VFS basics](vfs-basics.md) to understand how the engine loads content
- Read about [Widgets and Gadgets](widgets-and-gadgets.md) to understand Lua scripting
- Explore [basecontent](basecontent.md) for the default scripts provided by the engine
- Check out [Unit Types Basics](unit-types-basics.md) to define your first units

For API reference while developing, see the [Lua API documentation](/docs/lua-api/).

## Troubleshooting

### Engine can't find my game or map

1. Verify the file is in the correct `games/` or `maps/` directory
2. Check the archive extension (`.sd7`, `.sdz`, or `.sdd`)
3. Ensure `ModInfo.lua` exists at the root of your game
4. Try running with `--isolation` to limit to a single data directory

### No writable data directory

The engine needs a writable location for saves, replays, and logs. If you see this
error, see the [Write Directory](#write-directory) section above. Quick fixes:

- Run from a writable directory (not `/usr/bin` or similar)
- Use `--write-dir /path/to/dir` command-line flag
- Set `SPRING_WRITEDIR` environment variable
- Create `isolated.txt` for portable mode
- Configure `SpringData` in `springsettings.cfg` (Windows) or `~/.springrc` (Linux)

### Understanding the error messages

When in doubt, the engine sources contain the authoritative behavior.
Search the [`rts/System/FileSystem/`](https://github.com/beyond-all-reason/RecoilEngine/tree/master/rts/System/FileSystem)
directory for data directory handling code.
