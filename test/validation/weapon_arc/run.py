#!/usr/bin/env python3
# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
"""Run the BAR #3324 Wolverine regression in a fresh isolated directory."""

import argparse
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--engine", type=Path, required=True)
parser.add_argument("--game", type=Path, required=True, help="BAR checkout")
parser.add_argument("--map", type=Path, required=True, help="Quicksilver Remake 1.24 archive")
parser.add_argument("--expect", choices=("pass", "fail"), default="pass")
args = parser.parse_args()
engine, game, map_file = (p.resolve(strict=True) for p in (args.engine, args.game, args.map))
root = Path(tempfile.mkdtemp(prefix="weapon-arc-"))
overlay = root / "games" / "arc.sdd"
gadgets = overlay / "luarules" / "gadgets"
gadgets.mkdir(parents=True)
(root / "games" / "bar.sdd").symlink_to(game, target_is_directory=True)
(root / "maps").mkdir()
(root / "maps" / map_file.name).symlink_to(map_file)
shutil.copyfile(Path(__file__).with_name("gadget.lua"), gadgets / "dbg_weapon_arc.lua")
(overlay / "modinfo.lua").write_text(
    "return {name='Weapon arc regression', version='1', modtype=1, "
    "depend={'Beyond All Reason $VERSION'}}\n"
)
(root / "springsettings.cfg").write_text("WorkerThreadCount = 4\n")
script = root / "script.txt"
script.write_text("""[game]
{
 mapname=Quicksilver Remake 1.24;
 gametype=Weapon arc regression 1;
 myplayername=ArcTest;
 ishost=1;
 hostip=127.0.0.1;
 hostport=0;
 startpostype=0;
 gamestartdelay=0;
 fixedrngseed=1;
 nohelperais=0;
 recorddemo=0;
 [modoptions] { groundattackstallreproducer=1; deathmode=neverend; }
 [allyteam0] { numallies=0; }
 [team0] { teamleader=0; allyteam=0; startposx=2166; startposz=5477; }
 [player0] { team=0; name=ArcTest; }
}
""")
log = root / "engine.log"
print(f"Results: {root}", flush=True)
with log.open("w") as output:
    result = subprocess.run(
        [str(engine), "--isolation", "--write-dir", str(root), str(script)],
        stdout=output, stderr=subprocess.STDOUT, timeout=180,
    )
text = log.read_text(errors="replace")
for line in text.splitlines():
    if "WEAPON_ARC_" in line or "GROUND_ATTACK_STALL_RESULT" in line:
        print(line)
match = re.search(r"WEAPON_ARC_RESULT, (PASS|FAIL), projectiles, (\d+)", text)
if result.returncode or match is None or match[1].lower() != args.expect:
    raise SystemExit(f"Unexpected result; inspect {log}")
