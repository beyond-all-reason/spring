#!/usr/bin/env python3
"""
STREFLOP Float Test Result Comparator

Reads two binary result files produced by streflop-float-test and reports
matching/mismatching results with ULP difference analysis.

Supports streaming comparison for large files (millions/billions of tests).

Usage:
    python3 compare_results.py <reference.bin> <test.bin>
    python3 compare_results.py --hash <file.bin>          # extract/compute hash
"""

import struct
import sys
import os
from collections import defaultdict


# FNV-1a 64-bit (must match C++ implementation exactly)
FNV_OFFSET_BASIS = 0xcbf29ce484222325
FNV_PRIME        = 0x100000001b3
MASK64           = 0xFFFFFFFFFFFFFFFF


def fnv1a_init():
    return FNV_OFFSET_BASIS


def fnv1a_feed_bytes(h, data):
    """Feed raw bytes into FNV-1a hash."""
    for b in data:
        h = ((h ^ b) * FNV_PRIME) & MASK64
    return h


def read_header(f):
    """Read binary file header. Returns (header_dict, data_start_offset)."""
    magic = f.read(4)
    if magic != b"SFLT":
        raise ValueError(f"bad magic {magic!r}, expected b'SFLT'")

    version = struct.unpack("<I", f.read(4))[0]
    if version != 1:
        raise ValueError(f"unsupported version {version}")

    # Mode string (null-terminated)
    mode = b""
    while True:
        c = f.read(1)
        if c == b"\x00" or not c:
            break
        mode += c
    mode = mode.decode("utf-8")

    # Arch string (null-terminated)
    arch = b""
    while True:
        c = f.read(1)
        if c == b"\x00" or not c:
            break
        arch += c
    arch = arch.decode("utf-8")

    count = struct.unpack("<I", f.read(4))[0]

    return {"mode": mode, "arch": arch, "version": version, "count": count}


def read_record(f):
    """Read one binary record. Returns (test_id, prec, result_bits) or None at EOF."""
    data = f.read(13)
    if len(data) < 13:
        return None
    test_id = struct.unpack_from("<I", data, 0)[0]
    prec = chr(data[4])
    result_bits = struct.unpack_from("<Q", data, 5)[0]
    return test_id, prec, result_bits


def compute_hash(path):
    """Compute FNV-1a hash of all records in a binary file (matches C++ output)."""
    with open(path, "rb") as f:
        header = read_header(f)
        h = fnv1a_init()
        count = 0
        while True:
            data = f.read(13)
            if len(data) < 13:
                break
            # Hash exactly what C++ hashes: id(4) + prec(1) + result(8)
            h = fnv1a_feed_bytes(h, data)
            count += 1
            if count % 10_000_000 == 0:
                print(f"  ... {count:,} records hashed", file=sys.stderr)
    return header, h, count


def ulp_diff_f32(bits_a, bits_b):
    """Compute ULP distance between two float32 bit patterns."""
    a = bits_a & 0xFFFFFFFF
    b = bits_b & 0xFFFFFFFF

    # Handle NaN
    if (a & 0x7F800000) == 0x7F800000 and (a & 0x007FFFFF) != 0:
        return None
    if (b & 0x7F800000) == 0x7F800000 and (b & 0x007FFFFF) != 0:
        return None

    if a & 0x80000000:
        a = 0x80000000 - (a & 0x7FFFFFFF)
    else:
        a = a + 0x80000000
    if b & 0x80000000:
        b = 0x80000000 - (b & 0x7FFFFFFF)
    else:
        b = b + 0x80000000

    return abs(a - b)


def ulp_diff_f64(bits_a, bits_b):
    """Compute ULP distance between two float64 bit patterns."""
    a = bits_a & 0xFFFFFFFFFFFFFFFF
    b = bits_b & 0xFFFFFFFFFFFFFFFF

    if (a & 0x7FF0000000000000) == 0x7FF0000000000000 and (a & 0x000FFFFFFFFFFFFF) != 0:
        return None
    if (b & 0x7FF0000000000000) == 0x7FF0000000000000 and (b & 0x000FFFFFFFFFFFFF) != 0:
        return None

    if a & 0x8000000000000000:
        a = 0x8000000000000000 - (a & 0x7FFFFFFFFFFFFFFF)
    else:
        a = a + 0x8000000000000000
    if b & 0x8000000000000000:
        b = 0x8000000000000000 - (b & 0x7FFFFFFFFFFFFFFF)
    else:
        b = b + 0x8000000000000000

    return abs(a - b)


def bits_to_float(bits):
    return struct.unpack("<f", struct.pack("<I", bits & 0xFFFFFFFF))[0]


def bits_to_double(bits):
    return struct.unpack("<d", struct.pack("<Q", bits & 0xFFFFFFFFFFFFFFFF))[0]


def read_text_categories(path):
    """Try to read the .txt file to get category/operation info per test ID."""
    txt_path = path.rsplit(".", 1)[0] + ".txt"
    info = {}
    try:
        with open(txt_path, "r") as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("#"):
                    continue
                parts = line.split()
                if len(parts) >= 4:
                    try:
                        tid = int(parts[0])
                        cat = parts[2]
                        op = parts[3]
                        info[tid] = (cat, op)
                    except (ValueError, IndexError):
                        pass
    except FileNotFoundError:
        pass
    return info


def stream_compare(ref_path, test_path):
    """Stream-compare two binary files record by record. Memory-efficient for any scale."""
    ref_size = os.path.getsize(ref_path)
    test_size = os.path.getsize(test_path)

    with open(ref_path, "rb") as ref_f, open(test_path, "rb") as test_f:
        ref_header = read_header(ref_f)
        test_header = read_header(test_f)

        print(f"Reference: {ref_header['mode']} on {ref_header['arch']} ({ref_header['count']:,} tests)")
        print(f"Test:      {test_header['mode']} on {test_header['arch']} ({test_header['count']:,} tests)")
        print()

        ref_count = ref_header["count"]
        test_count = test_header["count"]

        if ref_count != test_count:
            print(f"WARNING: Different test counts (ref={ref_count:,}, test={test_count:,})")
            print(f"         Comparing first {min(ref_count, test_count):,} records")
            print()

        compare_count = min(ref_count, test_count)
        large_file = compare_count > 100_000

        matching = 0
        mismatched = 0
        nan_both = 0
        first_mismatches = []  # store first N mismatches for reporting
        max_stored_mismatches = 50

        # Try to load category info (only for small files with text output)
        cat_info = {}
        if not large_file:
            cat_info = {**read_text_categories(test_path), **read_text_categories(ref_path)}

        # Progress reporting
        progress_interval = 10_000_000 if compare_count > 100_000_000 else (
            1_000_000 if compare_count > 1_000_000 else 0xFFFFFFFF)

        for i in range(compare_count):
            ref_rec = read_record(ref_f)
            test_rec = read_record(test_f)

            if ref_rec is None or test_rec is None:
                print(f"WARNING: Unexpected EOF at record {i:,}")
                break

            ref_id, ref_prec, ref_bits = ref_rec
            test_id, test_prec, test_bits = test_rec

            if ref_id != test_id:
                # IDs out of sync — files were generated with different parameters
                if i == 0:
                    print("ERROR: Test IDs don't match (ref_id={}, test_id={})".format(ref_id, test_id))
                    print("       Files must be generated with the same -n parameter")
                    return 1
                # For later records, just note the mismatch
                mismatched += 1
                if len(first_mismatches) < max_stored_mismatches:
                    first_mismatches.append({
                        "id": ref_id, "reason": f"ID mismatch: ref={ref_id}, test={test_id}"
                    })
                continue

            if ref_prec != test_prec:
                mismatched += 1
                if len(first_mismatches) < max_stored_mismatches:
                    first_mismatches.append({
                        "id": ref_id, "reason": f"precision mismatch: ref={ref_prec}, test={test_prec}"
                    })
                continue

            if ref_bits == test_bits:
                matching += 1
                if (i + 1) % progress_interval == 0:
                    pct = 100.0 * (i + 1) / compare_count
                    print(f"  Progress: {i+1:,} / {compare_count:,} ({pct:.1f}%) — {mismatched} mismatches so far")
                continue

            # Check NaN
            prec = ref_prec
            if prec == "F":
                ref_nan = (ref_bits & 0x7F800000) == 0x7F800000 and (ref_bits & 0x007FFFFF) != 0
                test_nan = (test_bits & 0x7F800000) == 0x7F800000 and (test_bits & 0x007FFFFF) != 0
            else:
                ref_nan = (ref_bits & 0x7FF0000000000000) == 0x7FF0000000000000 and (ref_bits & 0x000FFFFFFFFFFFFF) != 0
                test_nan = (test_bits & 0x7FF0000000000000) == 0x7FF0000000000000 and (test_bits & 0x000FFFFFFFFFFFFF) != 0

            if ref_nan and test_nan:
                nan_both += 1
                matching += 1
                continue

            # Genuine mismatch
            mismatched += 1
            cat, op = cat_info.get(ref_id, ("?", "?"))

            if len(first_mismatches) < max_stored_mismatches:
                if prec == "F":
                    ulp = ulp_diff_f32(ref_bits, test_bits)
                    first_mismatches.append({
                        "id": ref_id, "op": op, "cat": cat,
                        "ref_hex": f"{ref_bits & 0xFFFFFFFF:08X}",
                        "test_hex": f"{test_bits & 0xFFFFFFFF:08X}",
                        "ref_val": f"{bits_to_float(ref_bits):.9g}",
                        "test_val": f"{bits_to_float(test_bits):.9g}",
                        "ulp": ulp,
                    })
                else:
                    ulp = ulp_diff_f64(ref_bits, test_bits)
                    first_mismatches.append({
                        "id": ref_id, "op": op, "cat": cat,
                        "ref_hex": f"{ref_bits:016X}",
                        "test_hex": f"{test_bits:016X}",
                        "ref_val": f"{bits_to_double(ref_bits):.17g}",
                        "test_val": f"{bits_to_double(test_bits):.17g}",
                        "ulp": ulp,
                    })

            if (i + 1) % progress_interval == 0:
                pct = 100.0 * (i + 1) / compare_count
                print(f"  Progress: {i+1:,} / {compare_count:,} ({pct:.1f}%) — {mismatched} mismatches so far")

    # --- Summary ---
    total = matching + mismatched
    print(f"{'='*60}")
    print(f"COMPARISON SUMMARY")
    print(f"{'='*60}")
    print(f"  Common tests:    {total:,}")
    print(f"  Matching:        {matching:,} ({100*matching/total:.1f}%)" if total else "  Matching:        0")
    print(f"  Mismatched:      {mismatched:,} ({100*mismatched/total:.1f}%)" if total else "  Mismatched:      0")
    if nan_both:
        print(f"  NaN-both:        {nan_both:,} (counted as match)")
    print()

    if not mismatched:
        print("RESULT: BIT-EXACT MATCH")
        return 0

    # Show mismatches
    print(f"FIRST {len(first_mismatches)} MISMATCHES:")
    print(f"{'-'*60}")
    for m in first_mismatches:
        if "reason" in m:
            print(f"  ID {m['id']:>8}  {m['reason']}")
        else:
            ulp_str = f"ULP={m['ulp']}" if m.get("ulp") is not None else "ULP=N/A"
            print(f"  ID {m['id']:>8}  [{m.get('cat','?')}] {m.get('op','?'):<14s}  "
                  f"ref={m.get('ref_hex','?')}  test={m.get('test_hex','?')}  {ulp_str}")
            if "ref_val" in m:
                print(f"             ref_val={m['ref_val']}  test_val={m['test_val']}")
    if mismatched > len(first_mismatches):
        print(f"  ... and {mismatched - len(first_mismatches):,} more")

    print()
    print(f"RESULT: {mismatched:,} MISMATCHES FOUND")
    return 1


def hash_mode(path):
    """Compute and print hash for a single binary file."""
    print(f"Computing hash for: {path}")
    fsize = os.path.getsize(path)
    if fsize > 1_000_000_000:
        print(f"  File size: {fsize / 1e9:.2f} GB")
    elif fsize > 1_000_000:
        print(f"  File size: {fsize / 1e6:.1f} MB")
    else:
        print(f"  File size: {fsize / 1e3:.1f} KB")

    header, h, count = compute_hash(path)
    print()
    print(f"{'='*60}")
    print(f"HASH RESULT")
    print(f"{'='*60}")
    print(f"  File:  {path}")
    print(f"  Mode:  {header['mode']} on {header['arch']}")
    print(f"  Tests: {count:,}")
    print(f"  Hash:  {h:016X}")
    print(f"{'='*60}")
    return 0


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <reference.bin> <test.bin>")
        print(f"       {sys.argv[0]} --hash <file.bin>")
        print()
        print("Compares two STREFLOP float test result files and reports")
        print("matching/mismatching results with ULP difference analysis.")
        print()
        print("Options:")
        print("  --hash <file.bin>   Compute and print verification hash")
        sys.exit(1)

    if sys.argv[1] == "--hash":
        if len(sys.argv) < 3:
            print("ERROR: --hash requires a file argument")
            sys.exit(1)
        sys.exit(hash_mode(sys.argv[2]))

    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <reference.bin> <test.bin>")
        sys.exit(1)

    ref_path = sys.argv[1]
    test_path = sys.argv[2]

    print(f"Reference: {ref_path}")
    print(f"Test:      {test_path}")
    print()

    sys.exit(stream_compare(ref_path, test_path))


if __name__ == "__main__":
    main()
