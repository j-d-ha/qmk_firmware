#!/usr/bin/env bash

set -euo pipefail

KEYBOARD="crkbd/rev1"
KEYMAP="hk"
BUILD_DIR=".build"
BASE_HEX="crkbd_rev1_hk.hex"

make "${KEYBOARD}:${KEYMAP}" SIDE=right "$@"
cp "${BUILD_DIR}/${BASE_HEX}" "${BUILD_DIR}/crkbd_rev1_hk_right.hex"

make "${KEYBOARD}:${KEYMAP}" SIDE=left "$@"
cp "${BUILD_DIR}/${BASE_HEX}" "${BUILD_DIR}/crkbd_rev1_hk_left.hex"

printf 'Built:\n  %s\n  %s\n' \
  "${BUILD_DIR}/crkbd_rev1_hk_right.hex" \
  "${BUILD_DIR}/crkbd_rev1_hk_left.hex"
