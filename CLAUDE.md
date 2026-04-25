# Project Notes (Agents)

## Scope
- Repo: Corne firmware work.
- Main target: `keyboards/crkbd/rev1_rp2040`.
- Pointer stack: HolyKeebs style.

## Rules
- Right trackpoint = cursor move.
- Left trackball = scroll.
- Keep pointer logic in `users/holykeebs`.
- Keep board wiring in `keyboards/*`.

## Build
- Script: `keyboards/crkbd/keymaps/hk/build_split.sh`.
- Right file: `.build/crkbd_rev1_rp2040_hk_right.uf2`.
- Left file: `.build/crkbd_rev1_rp2040_hk_left.uf2`.

## Flash
- Flash right file to right half.
- Flash left file to left half.

## Source of truth
- Working baseline: `~/Repos/Other/allie-cat-keeb-vial`.
- Core HolyKeebs repo: `~/Repos/Other/holykeebs_qmk_firmware`.
- Match behavior first. Then improve.
