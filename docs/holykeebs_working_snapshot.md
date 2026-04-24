# HolyKeebs Working Snapshot

Updated: 2026-04-24 (Option B progress)

This is a checkpoint document so we can continue implementation without losing context.

## Current repo state

- Repo: `/Users/jonasha/Repos/Other/vial-hk-custom`
- Branch: `feat/add-hk-pointers`
- Latest base integration commit: `94b2f11f68` (`Add HolyKeebs split pointing support for Corne rev1`)
- Option B target commit added: `eb4fcc72a0` (`Add RP2040 Corne target for HK split builds`)
- Current uncommitted files: none expected after committing this snapshot.

## Option B status (new RP2040 target)

- Added new target:
  - `keyboards/crkbd/rev1_rp2040/keyboard.json`
  - `keyboards/crkbd/rev1_rp2040/halconf.h`
- Preserved existing AVR target:
  - `keyboards/crkbd/rev1/keyboard.json` remains unchanged.
- HK split helper default now targets RP2040:
  - `keyboards/crkbd/keymaps/hk/build_split.sh` defaults to `KEYBOARD=crkbd/rev1_rp2040`.
- RP2040 support defaults updated in HK userspace:
  - ChibiOS split uses `SERIAL_DRIVER=vendor`.
  - Non-AVR split default serial TX pin is `GP1`.
  - PS/2 defaults remain `GP2/GP3` for non-AVR and `D1/D0` for AVR.

Verified builds:

- `make crkbd/rev1_rp2040:default`
- `make crkbd/rev1_rp2040:hk SIDE=right`
- `make crkbd/rev1_rp2040:hk SIDE=left`
- `./keyboards/crkbd/keymaps/hk/build_split.sh`

Generated split artifacts:

- `.build/crkbd_rev1_rp2040_hk_right.uf2`
- `.build/crkbd_rev1_rp2040_hk_left.uf2`

## What is already implemented (committed)

- HolyKeebs userspace scaffold and runtime in `users/holykeebs/*`.
- PS/2 pointing plumbing added:
  - `builddefs/common_features.mk` includes `POINTING_DEVICE_DRIVER=ps2` path.
  - `quantum/pointing_device/pointing_device.h` includes `drivers/sensors/ps2.h` branch.
  - `drivers/sensors/ps2.[ch]` wrapper driver added.
  - `drivers/ps2/ps2_mouse.[ch]` extended with `ps2_mouse_read(...)` and trackpoint register helpers.
- Corne HK keymap added at `keyboards/crkbd/keymaps/hk/*` with 4-layer layout and HK tuning keys.
- Split build helper script added: `keyboards/crkbd/keymaps/hk/build_split.sh`.

## Important behavior constraints

- Do not enable `PS2_MOUSE_ENABLE` for pointing-driver trackpoint builds, or PS/2 packets can be consumed outside the pointing pipeline.
- AVR `crkbd/rev1` builds produce `.hex` artifacts (correct behavior for current board definition).
- RP2040 builds produce `.uf2` artifacts only when the selected target is actually RP2040.

## RP2040 findings from recent testing

- `crkbd/rev4_0` and `crkbd/rev4_1` in this repo are RP2040 revisions.
- `crkbd/rev4_1` build target requires subtype path (`crkbd/rev4_1/standard` or `crkbd/rev4_1/mini`).
- A likely runtime failure was identified on rev4 test firmware:
  - default PS/2 pins `GP2/GP3` conflict with rev4 matrix pins in `keyboards/crkbd/rev4_1/info.json`.
- Possible additional mismatch risk: flashing `standard` firmware onto `mini` hardware (or vice versa).

## Reference repo lessons (`allie-cat-keeb-vial`)

Compared repo: `/Users/jonasha/Repos/Other/allie-cat-keeb-vial`

Key lesson:

- Their `keyboards/crkbd/rev1/keyboard.json` is RP2040-based (GP pins, RP2040 processor/bootloader).
- That is why `make crkbd/rev1:via ...` generates `.uf2` there.
- In this repo, `keyboards/crkbd/rev1/keyboard.json` is AVR/Pro Micro, so `crkbd/rev1` correctly generates `.hex`.

Vial lesson:

- Their Vial recognition comes from actual Vial assets/config:
  - `VIAL_ENABLE=yes`
  - `VIAL_KEYBOARD_UID` and unlock combo macros
  - `keyboards/crkbd/keymaps/via/vial.json`
- Current HK integration here is VIA-enabled, not full Vial-enabled.

## Recommended direction before next code changes

Choose one explicit hardware path:

1. Keep current AVR `crkbd/rev1` target (hex artifacts), or
2. Add a new RP2040 target in this repo (recommended for clean migration), or
3. Repurpose this repo's `crkbd/rev1` to RP2040 (possible, but confusing and breaks AVR expectations).

If RP2040 remains the goal, define non-conflicting PS/2 pins for the exact board subtype first.

## Quick command reminders

- Current default split helper (present state):
  - `./keyboards/crkbd/keymaps/hk/build_split.sh`
- Manual build pattern:
  - `make <keyboard>:hk SIDE=right`
  - `make <keyboard>:hk SIDE=left`
