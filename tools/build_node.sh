#!/usr/bin/env bash
# ARMOR-HMI - builds the firmware of one panel in the official ESP-IDF container.
# Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#
#   tools/build_node.sh generic                # the UNIVERSAL image for the Waveshare ESP32-S3-Touch-LCD-7C-BOX: no panel's data inside, writes dist/generic-lcd7box.bin
#   tools/build_node.sh salon                  # reads secrets/salon.conf, writes dist/salon-lcd7box.bin
#
# The only board is lcd7box (the Waveshare ESP32-S3-Touch-LCD-7C-BOX); the profile decides the flash size, the PSRAM and the pin table. An image is for ONE board: flash it only there.
#
# secrets/<node>.conf holds that panel's own settings (its identity, its broker password, and anything else that differs from the defaults) as CONFIG_ lines; write it by hand
# from secrets/node.conf.example. It is git-ignored: a password never enters the repository. Run it from Linux, or from WSL on Windows (Docker needed). The result is ONE merged
# image, flashed at address 0.
#
# The build needs Internet the first time (ESP-IDF's component manager fetches LVGL, esp_lvgl_port, the GT911 driver and esp_codec_dev, see main/idf_component.yml).
set -euo pipefail
NODE="${1:-}"
BOARD="${2:-lcd7box}"
[[ "$NODE" =~ ^[a-z0-9][a-z0-9_-]{0,63}$ ]] || { echo "usage: build_node.sh NODE_ID [lcd7box]   (lowercase letters, digits, - and _)" >&2; exit 2; }
[[ "$BOARD" == "lcd7box" ]] || { echo "the board is lcd7box, not '$BOARD'" >&2; exit 2; }
IMAGE="$NODE-$BOARD"
IDF_IMAGE="${ARMOR_IDF_IMAGE:-espressif/idf:v5.4.2}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CONF="secrets/$NODE.conf"
DEFAULTS="sdkconfig.defaults;sdkconfig.board.$BOARD;$CONF"
if [[ "$NODE" == "generic" ]]; then
  # The universal image: the same firmware for every panel. Name, address, Wi-Fi, server and users are set in each panel's own web page. secrets/generic.conf, if it exists
  # (write it by hand), holds the fleet secret from which each board's set-up code is derived using its MAC.
  DEFAULTS="sdkconfig.defaults;sdkconfig.board.$BOARD"
  [[ -f "$ROOT/secrets/generic.conf" ]] && DEFAULTS="$DEFAULTS;secrets/generic.conf"
else
  [[ -f "$ROOT/$CONF" ]] || { echo "missing $CONF - copy secrets/node.conf.example to $CONF and edit it, or build the universal image: tools/build_node.sh generic" >&2; exit 1; }
  grep -q "CONFIG_ARMOR_NODE_ID=\"$NODE\"" "$ROOT/$CONF" || { echo "$CONF must set CONFIG_ARMOR_NODE_ID=\"$NODE\"" >&2; exit 1; }
fi

DOCKER="docker"
docker info >/dev/null 2>&1 || DOCKER="sudo docker"
mkdir -p "$ROOT/dist" "$ROOT/build"
# The build runs as the caller so no file in the tree ends up owned by root; HOME is a scratch directory for idf.py.
# The machine keeps answering while it builds: the container sees only 4 cores (ARMOR_BUILD_CPUSET, default 0-3).
$DOCKER run --rm --cpuset-cpus="${ARMOR_BUILD_CPUSET:-0-3}" -u "$(id -u):$(id -g)" -e HOME=/tmp -v "$ROOT":/project -w /project "$IDF_IMAGE" bash -c "
  set -e
  BUILD=build/$IMAGE
  idf.py -B \$BUILD -D SDKCONFIG=\$BUILD/sdkconfig -D 'SDKCONFIG_DEFAULTS=$DEFAULTS' set-target esp32s3 >/dev/null
  idf.py -B \$BUILD -D SDKCONFIG=\$BUILD/sdkconfig -D 'SDKCONFIG_DEFAULTS=$DEFAULTS' build
  cd \$BUILD
  python -m esptool --chip esp32s3 merge_bin -o /project/dist/$IMAGE.bin @flash_args
"
ls -l "$ROOT/dist/$IMAGE.bin"
sha256sum "$ROOT/dist/$IMAGE.bin" | cut -d' ' -f1 | sed "s/^/sha256 /"
echo "flash it (to a $BOARD board only) with: python -m esptool --chip esp32s3 -p COMx write_flash 0x0 dist/$IMAGE.bin"
