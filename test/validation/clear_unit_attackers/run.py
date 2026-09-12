#!/usr/bin/env python3
# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
"""Validate ClearUnitAttackers in an isolated minimal game."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile
import zipfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("engine", type=Path)
parser.add_argument("map_archive", type=Path)
parser.add_argument("--map-name", default="Quicksilver Remake 1.24")
args = parser.parse_args()
engine = args.engine.resolve(strict=True)
map_archive = args.map_archive.resolve(strict=True)
if any(c in args.map_name for c in ";{}\r\n"):
    parser.error("invalid map name")
data = Path(tempfile.mkdtemp(prefix="recoil-clear-attackers-"))
(data / "games").mkdir()
(data / "maps").mkdir()
(data / "maps" / map_archive.name).symlink_to(map_archive)
game = Path(__file__).parent / "game"
with zipfile.ZipFile(data / "games" / "clear-attackers.sdz", "w") as archive:
    for source in sorted(game.rglob("*")):
        if source.is_file():
            archive.write(source, source.relative_to(game))
(data / "springsettings.cfg").write_text(
    "LuaUI = 0\nLuaMenu = 0\nNoSound = 1\nWorkerThreadCount = 2\nHardwareThreadCount = 4\n"
)
script = data / "startscript.txt"
script.write_text(f"""[GAME]
{{
    MapName={args.map_name};
    GameType=Clear attackers validation 1;
    GameStartDelay=0;
    StartPosType=0;
    RecordDemo=0;
    MyPlayerName=TestRunner;
    IsHost=1;
    HostIP=127.0.0.1;
    HostPort=0;
    FixedRNGSeed=1;
    [MODOPTIONS] {{ LuaGaia=0; }}
    [ALLYTEAM0] {{ NumAllies=0; }}
    [ALLYTEAM1] {{ NumAllies=0; }}
    [ALLYTEAM2] {{ NumAllies=0; }}
    [TEAM0] {{ AllyTeam=0; TeamLeader=0; }}
    [TEAM1] {{ AllyTeam=1; TeamLeader=0; }}
    [TEAM2] {{ AllyTeam=2; TeamLeader=0; }}
    [PLAYER0] {{ Name=TestRunner; Team=0; }}
}}
""")
print(f"Test data and logs: {data}", flush=True)
env = {k: v for k, v in os.environ.items() if k not in ("SPRING_DATADIR", "SPRING_WRITEDIR")}
with (data / "console.log").open("w") as log:
    result = subprocess.run(
        [str(engine), "--isolation", "--write-dir", str(data), str(script)],
        cwd=data, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=120,
    )
output = (data / "console.log").read_text(errors="replace")
for line in output.splitlines():
    if "CLEAR_ATTACKERS" in line:
        print(line)
if (result.returncode or "CLEAR_ATTACKERS PASS" not in output
        or "CLEAR_ATTACKERS UNSYNCED PASS" not in output
        or "CLEAR_ATTACKERS FAIL" in output or "RunCallInTraceback" in output):
    raise SystemExit(f"Validation failed (engine exit {result.returncode}); see {data / 'console.log'}")
