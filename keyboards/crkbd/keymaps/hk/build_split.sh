#!/usr/bin/env bash

set -euo pipefail

KEYBOARD="${KEYBOARD:-crkbd/rev1_rp2040}"
KEYMAP="hk"
BUILD_DIR=".build"

BASE_NAME="${KEYBOARD//\//_}_${KEYMAP}"

case "${KEYBOARD}" in
  crkbd/rev1_rp2040)
    ARTIFACT_EXT="uf2"
    ;;
  crkbd/rev4_0/*|crkbd/rev4_1/*)
    ARTIFACT_EXT="uf2"
    ;;
  *)
    ARTIFACT_EXT="hex"
    ;;
esac

BASE_ARTIFACT="${BASE_NAME}.${ARTIFACT_EXT}"

make "${KEYBOARD}:${KEYMAP}" SIDE=right "$@"
cp "${BUILD_DIR}/${BASE_ARTIFACT}" "${BUILD_DIR}/${BASE_NAME}_right.${ARTIFACT_EXT}"

make "${KEYBOARD}:${KEYMAP}" SIDE=left "$@"
cp "${BUILD_DIR}/${BASE_ARTIFACT}" "${BUILD_DIR}/${BASE_NAME}_left.${ARTIFACT_EXT}"

printf 'Built:\n  %s\n  %s\n' \
  "${BUILD_DIR}/${BASE_NAME}_right.${ARTIFACT_EXT}" \
  "${BUILD_DIR}/${BASE_NAME}_left.${ARTIFACT_EXT}"
