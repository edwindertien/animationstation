#!/usr/bin/env bash
# src/main.cpp is the master copy of the firmware. Run this after changing it to
# refresh the Arduino IDE sketch (arduino/animationstation/animationstation.ino).
set -euo pipefail
cd "$(dirname "$0")/.."
cp src/main.cpp arduino/animationstation/animationstation.ino
echo "arduino/animationstation/animationstation.ino updated from src/main.cpp"
