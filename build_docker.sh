#!/usr/bin/env bash
set -e

echo "=============================================="
echo "Building DangoPlayer Switch using Docker..."
echo "=============================================="

docker build -t dangoplayer-switch .
docker run --rm -v "$(pwd)":/project dangoplayer-switch make

echo "[SUCCESS] Build finished! Output: DangoPlayerSwitch.nro"
