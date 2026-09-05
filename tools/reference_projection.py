#!/usr/bin/env python3
"""Independent reference for the Hayes pose-projection challenge.

Recomputes every 2D coordinate from the raw data using nothing but the
standard library, then compares against the CSV the Godot tool exported.
Third opinion: if this and both C# implementations agree, the numbers are
almost certainly right.
"""

import csv
import math
import os
import sys

TOLERANCE = 0.01
JOINTS = 14
PRINCIPAL = 500.0
METHODS = ("manual_pinhole", "godot_unproject")
CSV_FIELDS = ["frame", "joint_id", "joint_name", "method", "u", "v"]


def load(data_dir):
    focal = float(open(os.path.join(data_dir, "focal.txt")).read().strip())
    rows = []
    for line in open(os.path.join(data_dir, "poses.txt")):
        line = line.strip()
        if line:
            rows.append([float(v) for v in line.split()])
    return focal, rows


def sub(a, b):
    return [a[0] - b[0], a[1] - b[1], a[2] - b[2]]


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def cross(a, b):
    return [a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0]]


def unit(a):
    n = math.sqrt(dot(a, a))
    return [a[0] / n, a[1] / n, a[2] / n]


def project_frame(row, focal):
    camera = row[0:3]
    joints = [row[3 + 3 * i:6 + 3 * i] for i in range(JOINTS)]

    centroid = [sum(j[k] for j in joints) / JOINTS for k in range(3)]

    forward = unit(sub(centroid, camera))
    right = unit(cross(forward, [0.0, 0.0, 1.0]))
    up = cross(right, forward)

    points = []
    for joint in joints:
        d = sub(joint, camera)
        z = dot(d, forward)
        u = focal * dot(d, right) / z + PRINCIPAL
        v = PRINCIPAL - focal * dot(d, up) / z
        points.append((u, v))
    return points


def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    focal, rows = load(os.path.join(root, "data"))

    expected = {}
    for frame, row in enumerate(rows):
        for joint, (u, v) in enumerate(project_frame(row, focal)):
            expected[(frame, joint)] = (u, v)

    path = os.path.join(root, "out", "coords_long.csv")
    if not os.path.exists(path):
        print("missing %s; run: godot-mono --headless --path . -- --export-csv" % path)
        return 1

    checked = 0
    worst = 0.0
    failures = []
    seen = set()
    with open(path, newline="") as handle:
        reader = csv.DictReader(handle)
        if reader.fieldnames != CSV_FIELDS:
            print("expected CSV fields %s, found %s" % (CSV_FIELDS, reader.fieldnames))
            return 1

        for line_number, record in enumerate(reader, start=2):
            try:
                frame = int(record["frame"])
                joint = int(record["joint_id"])
                method = record["method"]
                key = (frame, joint, method)
                want_u, want_v = expected[(frame, joint)]
                actual_u = float(record["u"])
                actual_v = float(record["v"])
            except (KeyError, TypeError, ValueError) as error:
                failures.append("line %d is malformed: %s" % (line_number, error))
                continue

            if method not in METHODS:
                failures.append("line %d has unexpected method %s" % (line_number, method))
            elif key in seen:
                failures.append("line %d duplicates frame %d joint %d method %s"
                                % (line_number, frame, joint, method))
            else:
                seen.add(key)

            du = abs(actual_u - want_u)
            dv = abs(actual_v - want_v)
            worst = max(worst, du, dv)
            checked += 1
            if du > TOLERANCE or dv > TOLERANCE:
                failures.append("frame %d joint %d method %s: du=%.4f dv=%.4f"
                                % (frame, joint, method, du, dv))

    expected_keys = {
        (frame, joint, method)
        for frame in range(len(rows))
        for joint in range(JOINTS)
        for method in METHODS
    }
    missing = sorted(expected_keys - seen)
    extra = sorted(seen - expected_keys)
    if missing:
        failures.append("missing %d expected frame/joint/method rows (first: %s)"
                        % (len(missing), missing[0]))
    if extra:
        failures.append("found %d unexpected frame/joint/method rows (first: %s)"
                        % (len(extra), extra[0]))

    expected_count = len(expected_keys)
    if checked != expected_count:
        failures.append("expected %d rows, checked %d" % (expected_count, checked))

    for line in failures:
        print(line)
    if failures:
        print("FAIL: %d issue(s) across %d checked rows" % (len(failures), checked))
        return 1

    print("OK: %d rows match the reference, worst deviation %.6f px" % (checked, worst))
    return 0


if __name__ == "__main__":
    sys.exit(main())
