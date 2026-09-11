#!/usr/bin/env python3
# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
"""Prepare an isolated accelerated replay using a local BAR checkout (demo v5)."""
import argparse, gzip, json, pathlib, re, struct, zlib, os, shutil
p = argparse.ArgumentParser()
p.add_argument('replay', type=pathlib.Path)
p.add_argument('output', type=pathlib.Path)
p.add_argument('--bar', type=pathlib.Path, required=True)
p.add_argument('--assets', type=pathlib.Path, required=True)
p.add_argument('--engine', type=pathlib.Path, required=True)
p.add_argument('--mode', choices=['native', 'fallback', 'lua'], required=True)
p.add_argument('--frames', type=int, default=36000)
a = p.parse_args()
a.output.mkdir(parents=True, exist_ok=True)
for name, target in [('maps', a.assets / 'maps')]:
    dst = a.output / name
    if not dst.exists():
        dst.symlink_to(target.resolve(), target_is_directory=True)
game = a.output / 'games' / 'attack-movement.sdd'
game.mkdir(parents=True, exist_ok=True)
# Directory archives do not recurse into symlinked subdirectories. Use actual
# directories and hardlinked files, retaining an independent modinfo below.
for dst in game.iterdir():
    if dst.is_symlink():
        dst.unlink()
shutil.copytree(a.bar, game, dirs_exist_ok=True, copy_function=lambda src, dst: os.link(src, dst) if not pathlib.Path(dst).exists() else dst, ignore=shutil.ignore_patterns('.git', 'modinfo.lua'))
(game / 'modinfo.lua').write_text((a.bar / 'modinfo.lua').read_text().replace('$VERSION', 'attack-movement'))
raw = bytearray(gzip.open(a.replay, 'rb').read())
assert raw[:15] == b'spring demofile' and struct.unpack_from('<i', raw, 16)[0] == 5
h = struct.unpack_from('<i', raw, 20)[0]
sz = struct.unpack_from('<i', raw, 304)[0]
streamsz = struct.unpack_from('<i', raw, 308)[0]
script = raw[h:h + sz].decode()

def rewrite(script):
    script = re.sub('(?im)^(\\s*gametype\\s*=)[^;]*;', '\\1Beyond All Reason attack-movement;', script)
    script = re.sub('(?im)^\\s*(gamehash|maphash)\\s*=[^;]*;', '', script)
    script = re.sub('(?im)^\\s*(maxspeed|minspeed)\\s*=[^;]*;', '', script)
    script = re.sub('(?i)(\\[modoptions\\]\\s*\\{)', lambda m: m[0] + f'\nmaxspeed=1000;\nminspeed=0.1;\nattackmovementmode={a.mode};\nattackmovementvalidation=replay;\nattackmovementendframe={a.frames};\n', script, count=1)
    return script
script = rewrite(script)
# Keep command and frame packets, compress wall-clock spacing only.
stream = raw[h + sz:h + sz + streamsz]
pos = 0
chunks = []
pivot = None
while pos < len(stream):
    t, n = struct.unpack_from('<fI', stream, pos)
    packet = stream[pos + 8:pos + 8 + n]
    pos += 8 + n
    # Old-build sync responses are not meaningful for this A/B test.
    if packet[0] == 33:
        continue
    if packet[0] == 52:
        compressed_size = struct.unpack_from('<H', packet, 3)[0]
        setup = rewrite(zlib.decompress(packet[5:5 + compressed_size]).decode())
        compressed = zlib.compress(setup.encode())
        tail = packet[5 + compressed_size:]
        packet = bytes([52]) + struct.pack('<HH', 5 + len(compressed) + len(tail), len(compressed)) + compressed + tail
    if pivot is None and packet[0] in (1, 2):
        pivot = t
    adjusted = t / 1000
    chunks.append(struct.pack('<fI', adjusted, len(packet)) + packet)
assert pos == len(stream)
stream = b''.join(chunks)
encoded = script.encode()
struct.pack_into('<i', raw, 304, len(encoded))
struct.pack_into('<i', raw, 308, len(stream))
(a.output / 'input.sdfz').write_bytes(gzip.compress(raw[:h] + encoded + stream + raw[h + sz + streamsz:], mtime=0))
(a.output / 'springsettings.cfg').write_text('LuaUI = 0\nSound = 0\nNetworkLossFactor = 0\nServerSleepTime = 0\nMaxSpeed = 1000\nMinSpeed = 1000\nWorkerThreadCount = 4\nHardwareThreadCount = 4\nPathingThreadCount = 1\nLogFlush = 0\n')
(a.output / 'manifest.json').write_text(json.dumps(vars(a), default=str, indent=2))
print(a.output / 'input.sdfz')
