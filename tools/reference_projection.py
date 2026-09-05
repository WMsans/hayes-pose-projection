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
    with open(path) as handle:
        for record in csv.DictReader(handle):
            key = (int(record["frame"]), int(record["joint_id"]))
            want_u, want_v = expected[key]
            du = abs(float(record["u"]) - want_u)
            dv = abs(float(record["v"]) - want_v)
            worst = max(worst, du, dv)
            checked += 1
            if du > TOLERANCE or dv > TOLERANCE:
                failures.append("frame %d joint %d method %s: du=%.4f dv=%.4f"
                                % (key[0], key[1], record["method"], du, dv))

    if checked != 2 * len(rows) * JOINTS:
        print("expected %d rows, checked %d" % (2 * len(rows) * JOINTS, checked))
        return 1

    for line in failures:
        print(line)
    if failures:
        print("FAIL: %d of %d rows disagree" % (len(failures), checked))
        return 1

    print("OK: %d rows match the reference, worst deviation %.6f px" % (checked, worst))
    return 0


if __name__ == "__main__":
    sys.exit(main())
