#!/usr/bin/env python3
# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
"""Run a prepared isolated integration input; reject failed or incomplete checks."""
import argparse
from pathlib import Path
import subprocess

p = argparse.ArgumentParser()
p.add_argument('engine', type=Path)
p.add_argument('root', type=Path)
p.add_argument('input', type=Path)
a = p.parse_args()
a.engine, a.root, a.input = a.engine.resolve(), a.root.resolve(), a.input.resolve()
with (a.root / 'console.log').open('w') as log:
    result = subprocess.run([str(a.engine), '--isolation', '--write-dir', str(a.root), str(a.input)], stdout=log, stderr=subprocess.STDOUT, timeout=600)
log = (a.root / 'infolog.txt').read_text(errors='replace')
for line in log.splitlines():
    if '[AttackValidation' in line:
        print(line)
assert result.returncode == 0, result.returncode
assert '[AttackValidation] PASS ' in log, 'missing completion marker'
assert '[AttackValidation] FAIL ' not in log, 'integration check failed'
assert 'callin=AttackCommandMovement trace=' not in log, 'attack callback error'
