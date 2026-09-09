#!/usr/bin/env python3
"""
Restore-identity diff: does save -> reload -> continue reproduce an uninterrupted run?

This is the save/load sync-safety question behind #1388 and replay seeking. It does not
test cross-platform determinism (that is the synctest CI's job); it tests whether a loaded
save continues the simulation bit-identically to a run that never reloaded.

Inputs are two demos of the SAME deterministic scenario:
  A = a continuous run that also saved at some frame K
  B = a run that loaded A's save at K and continued
The script reports the first recorded frame at/after K where their sync checksums differ.
Identical => the reload reproduced the timeline (sync-safe for this scenario). A divergence
=> the reload is not restore-identical, and the reported frame is where it first shows up.

Producing the inputs (any seeded, headless-survivable scenario works; BAR's synctest
scenario is a good one - fixed seed, no GL, self-quitting):
  # A: add a save to the scenario's debugcommands, e.g. |1000:save rrwsave|, then run it.
  spring-headless --isolation --write-dir DATADIR startscript-with-save.txt   # -> demo A + a .ssf
  # B: launch the engine on that save and let it continue.
  spring-headless --isolation --write-dir DATADIR DATADIR/Saves/rrwsave.ssf    # -> demo B

Checksums come from each demo's NETMSG_SYNCRESPONSE stream, read with this directory's
demotool (same idiom as count.py) rather than a separate demo parser. demotool is
EXCLUDE_FROM_ALL, so build it once first:
  cmake --build <build-dir> --target demotool

NETMSG_SYNCRESPONSE is recorded every ~60 frames (the engine throttles it), so the result
is a yes/no plus ~60-frame localization, not an exact frame. The synctest gadget's per-frame
synchash can't substitute here - its collector doesn't re-arm across a reload, so it cannot
capture run B. To find the actual cause of a divergence (which value, not just which frame),
diff a SYNCDEBUG/DumpState build at the seam; this tool only localizes.

Usage:
  restore_identity_diff.py [--demotool PATH] [--save-frame K] [--baseline FILE] DEMO_A DEMO_B
Exit codes:
  0  identical over the common range, OR informational (no baseline given)
  3  diverged earlier than the baseline (a regression) -- only with --baseline
  2  the two demos share no checksummed frames (cannot compare)
"""
import argparse
import re
import subprocess
import sys

# Matches demotool --dump lines, e.g.:
#   000123 NETMSG_SYNCRESPONSE: Playernum: 0 Framenum: 1020 Checksum: 1203041620
SYNCRESPONSE_RE = re.compile(r"NETMSG_SYNCRESPONSE:.*Framenum:\s*(-?\d+).*Checksum:\s*(\d+)")


def extract_checksums(demotool, demo):
    """frame -> checksum, via `demotool --dump <demo>`."""
    proc = subprocess.run([demotool, "--dump", demo],
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
    if proc.returncode != 0:
        sys.stderr.write(proc.stderr)
        raise SystemExit("demotool failed on %s (exit %d)" % (demo, proc.returncode))
    out = {}
    for line in proc.stdout.splitlines():
        m = SYNCRESPONSE_RE.search(line)
        if m:
            out[int(m.group(1))] = int(m.group(2))
    return out


def main():
    ap = argparse.ArgumentParser(description="Restore-identity diff for save/reload sync-safety")
    ap.add_argument("demo_a", help="continuous-run demo (.sdfz)")
    ap.add_argument("demo_b", help="reload-and-continue demo (.sdfz)")
    ap.add_argument("--demotool", default="demotool", help="path to the demotool binary (default: on PATH)")
    ap.add_argument("--save-frame", type=int, default=0,
                    help="only compare frames at/after this (the save point)")
    ap.add_argument("--baseline", default=None,
                    help="file holding a baseline first-divergent-frame; fail if this run diverges earlier "
                         "(handy for tracking progress on a sync-safe-save fix)")
    args = ap.parse_args()

    a = extract_checksums(args.demotool, args.demo_a)
    b = extract_checksums(args.demotool, args.demo_b)
    print("A %s: %d checksums, frames %s..%s" % (args.demo_a, len(a), min(a, default="-"), max(a, default="-")))
    print("B %s: %d checksums, frames %s..%s" % (args.demo_b, len(b), min(b, default="-"), max(b, default="-")))

    common = sorted(f for f in (set(a) & set(b)) if f >= args.save_frame)
    if not common:
        print("ERROR: no common checksummed frames at/after save frame %d - cannot compare" % args.save_frame)
        return 2
    lo, hi = common[0], common[-1]
    diverge = next((f for f in common if a[f] != b[f]), None)

    if diverge is None:
        print("OK: restore reproduced the timeline - identical on %d common frames [%d..%d]" % (len(common), lo, hi))
    else:
        print("DIVERGE at frame %d: A=%d B=%d (compared range [%d..%d]) - restore broke determinism here"
              % (diverge, a[diverge], b[diverge], lo, hi))
    print("METRIC first_divergent_frame=%s" % (diverge if diverge is not None else "none"))

    rc = 0
    if args.baseline:
        try:
            with open(args.baseline) as fh:
                base = fh.read().strip()
            base_fdf = None if base in ("", "none") else int(base)
        except FileNotFoundError:
            base_fdf = None
        cur = diverge if diverge is not None else (hi + 1)        # full match = best possible
        if base_fdf is not None and cur < base_fdf:
            print("REGRESSION: diverged at %d, earlier than baseline %d" % (cur, base_fdf))
            rc = 3
        else:
            print("no regression vs baseline (%s)" % (base_fdf if base_fdf is not None else "none"))
    return rc


if __name__ == "__main__":
    sys.exit(main())
