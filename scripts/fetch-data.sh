#!/usr/bin/env bash
# Downloads the challenge data. Not committed: the frames are Human3.6M imagery.
set -euo pipefail
mkdir -p data
curl -L -o data/Pose.zip https://ics.uci.edu/~wayne/research/students/Pose.zip
unzip -o -q data/Pose.zip -d data
test -f data/Pose/poses.txt && echo "data ready: data/Pose"
