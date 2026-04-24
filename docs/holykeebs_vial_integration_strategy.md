# HolyKeebs Pointing Integration Strategy for `vial-hk-custom`

This document describes how to bring HolyKeebs-style unified pointing support into this repository as one maintainable solution.

It is developer/AI implementation guidance, not end-user setup documentation.

## Goal

Integrate a single pointer architecture that supports:

- Pimoroni trackball
- PS/2 trackpoint
- Cirque (35/40)
- Azoteq TPS43/TPS65
- single-device and mixed split combinations

while preserving compatibility with this repo's current QMK/Vial core and keyboard structure.

## Executive summary

The correct approach is **layered integration**, not direct copy-paste of HolyKeebs userspace.

1. Reuse this repo's stock sensor drivers for Pimoroni, Cirque, and Azoteq.
2. Port HolyKeebs runtime/topology logic into a shared keyboard module.
3. Port the HolyKeebs PS/2 trackpoint delta (main blocker).
4. Keep board hardware configuration separate from shared pointer logic.

## Current state comparison

### In this repo (`vial-hk-custom`)

- No HolyKeebs userspace tree exists under `users/`.
- No `keyboards/holykeebs/` tree exists.
- Pointing core and split combined hooks already exist:
  - `quantum/pointing_device/pointing_device.c`
  - `quantum/pointing_device/pointing_device.h`
  - `quantum/split_common/transactions.c`
- Stock drivers already present:
  - `drivers/sensors/pimoroni_trackball.c`
  - `drivers/sensors/cirque_pinnacle_i2c.c`
  - `drivers/sensors/cirque_pinnacle_spi.c`
  - `drivers/sensors/azoteq_iqs5xx.c`
- `pointing_device_driver_t` uses `bool (*init)(void)` in this repo.

### In HolyKeebs repo (source behavior)

- Unified pointer behavior is implemented in userspace/runtime files, not primarily in sensor drivers.
- The major lower-level custom work is PS/2 trackpoint support (`drivers/sensors/ps2.*` and patched `drivers/ps2/ps2_mouse.*`).

## Key constraints that drive architecture

1. **API drift in pointing core**
   - This repo expects `bool` init for pointing drivers.
   - Any imported layer must preserve this ABI.

2. **PS/2 trackpoint path is incomplete in this repo**
   - `POINTING_DEVICE_DRIVER=ps2` is not currently a valid pointing driver type in `builddefs/common_features.mk`.
   - HolyKeebs relies on a PS/2 pointing wrapper and extra PS/2 mouse behavior.

3. **Board hardware mismatch**
   - Existing local `idank` boards are AVR/Pro Micro oriented.
   - HolyKeebs target boards are RP2040 oriented.
   - Shared logic can be common, hardware adapters cannot.

4. **Control plane mismatch (C keymaps vs JSON-centric keymaps)**
   - HolyKeebs tuning is exposed through custom `HK_*` keycodes in C keymaps.
   - Current local boards lean on JSON keymaps.

## Proposed integration architecture

Use a shared keyboard module plus board adapters.

### Shared module

Proposed location:

- `keyboards/idank/common/hk_pointing/`

Responsibilities:

- topology parsing and role assignment (main/peripheral)
- runtime report processing
- EEPROM-backed pointer settings
- optional split RPC state sync
- optional custom keycodes for tuning

### Board/revision adapters

Responsibilities:

- MCU-specific transport/pins
- per-board default rotations and sensor placement
- split serial/I2C/SPI/PS2 wiring

Each keyboard/revision should describe hardware, not duplicate pointer logic.

### Driver strategy

Use `POINTING_DEVICE_DRIVER=custom` as the integration seam for unified behavior.

Why this is preferred:

- keeps this repo's pointing core ABI unchanged
- allows one runtime layer to combine/transform reports consistently
- avoids invasive changes to `quantum/pointing_device/*`

## Device family porting requirements

### Pimoroni

- Reuse `drivers/sensors/pimoroni_trackball.c`.
- Port only topology/config/runtime behavior (rotation/defaults/optional helper logic).

### Cirque

- Reuse `drivers/sensors/cirque_pinnacle_*`.
- Port only size/gesture/rotation selection and runtime defaults.

### Azoteq TPS43/TPS65

- Reuse `drivers/sensors/azoteq_iqs5xx.c`.
- Port only TPS profile selection, rotation policy, press-and-hold enablement, and runtime defaults.

### Trackpoint (PS/2)

- This is the hard requirement.
- Port HolyKeebs PS/2 behavior into this repo:
  - side-aware init/read behavior for split combos
  - trackpoint register read/write helpers
  - timing/rate-limit behavior when mixed with faster devices
- Ensure compatibility with this repo's pointing driver ABI and build system.

## File-level migration map

### Source files to adapt from HolyKeebs runtime layer

- `/Users/jonasha/Repos/Other/holykeebs_qmk_firmware/users/holykeebs/holykeebs.c`
- `/Users/jonasha/Repos/Other/holykeebs_qmk_firmware/users/holykeebs/holykeebs.h`
- `/Users/jonasha/Repos/Other/holykeebs_qmk_firmware/users/holykeebs/pointing.h`
- `/Users/jonasha/Repos/Other/holykeebs_qmk_firmware/users/holykeebs/eeprom_config.h`

### Optional source files to adapt

- `/Users/jonasha/Repos/Other/holykeebs_qmk_firmware/users/holykeebs/rpc.c`
- `/Users/jonasha/Repos/Other/holykeebs_qmk_firmware/users/holykeebs/rpc.h`
- `/Users/jonasha/Repos/Other/holykeebs_qmk_firmware/users/holykeebs/hk_debug.c`
- `/Users/jonasha/Repos/Other/holykeebs_qmk_firmware/users/holykeebs/hk_debug.h`
- `/Users/jonasha/Repos/Other/holykeebs_qmk_firmware/users/holykeebs/pimoroni.c`
- `/Users/jonasha/Repos/Other/holykeebs_qmk_firmware/users/holykeebs/pimoroni.h`

### PS/2-specific files to port/rebase carefully

- `/Users/jonasha/Repos/Other/holykeebs_qmk_firmware/drivers/ps2/ps2_mouse.c`
- `/Users/jonasha/Repos/Other/holykeebs_qmk_firmware/drivers/ps2/ps2_mouse.h`
- `/Users/jonasha/Repos/Other/holykeebs_qmk_firmware/drivers/sensors/ps2.c`
- `/Users/jonasha/Repos/Other/holykeebs_qmk_firmware/drivers/sensors/ps2.h`

### Repo files likely to be touched here

- `builddefs/common_features.mk`
- `drivers/ps2/ps2_mouse.c`
- `drivers/ps2/ps2_mouse.h`
- `drivers/sensors/` (new `ps2.*` wrapper if using driver-registration path)
- `keyboards/idank/sweeq/*` (adapter wiring)
- `keyboards/idank/spankbd/*` (adapter wiring)

## Implementation phases

### Phase 1: Foundation (no new UI)

- Create shared module under `keyboards/idank/common/hk_pointing/`.
- Port state model + report processing + EEPROM config.
- Hook into `POINTING_DEVICE_DRIVER=custom`.
- Validate single-device path with one non-PS/2 sensor.

Success criteria:

- firmware builds
- pointer movement works
- no regressions to existing non-HK boards

### Phase 2: Split combined topology

- Add main/peripheral role logic and mixed-combo processing.
- Validate split combined behavior with existing split hooks.
- Keep per-pointer processing state isolated.

Success criteria:

- mixed split combinations produce stable, independent behavior

### Phase 3: Trackpoint enablement

- Port/rebase PS/2 trackpoint path.
- Add required builddefs and wrapper plumbing.
- Validate side-aware init/read and mixed timing behavior.

Success criteria:

- trackpoint builds and operates in single and mixed modes

### Phase 4: Controls and optional UI

- Add optional tuning keycode layer (`HK_*`) in C keymaps or an equivalent exposure path.
- Add optional OLED/RPC mirror integration.

Success criteria:

- runtime works without OLED
- optional UI works when enabled

## Risk register

### High risk

- PS/2 trackpoint port quality and timing interactions in mixed-device configs.
- Board wiring mismatch between AVR local boards and RP2040-origin assumptions.

### Medium risk

- Build-system drift when adding or wrapping PS/2 pointing support.
- Tuning-control exposure strategy in a JSON-keymap-centric environment.

### Low risk

- Reuse of Pimoroni/Cirque/Azoteq stock drivers.
- Split transaction API compatibility for optional RPC mirror.

## Quality gates before merge

Minimum test matrix:

- single Pimoroni
- single Cirque
- single TPS43 or TPS65
- single trackpoint
- one mixed combo (example: trackball + TPS43)
- one mixed combo involving trackpoint
- split left/right flash variants where applicable
- build with and without optional OLED/RPC code

## Known HolyKeebs issues to fix during port

Do not import these behaviors as-is:

- shared static accumulators that can bleed state across two pointers in combined mode
- scroll-buffer decrement underflow path
- drag-scroll type mismatch in helper API
- incomplete pointer-kind display handling for TPS65

## Open decisions

1. Should target support prioritize existing AVR `idank/*` definitions, new RP2040 revisions, or both?
2. Should trackpoint be integrated as:
   - `POINTING_DEVICE_DRIVER=ps2` builddefs extension, or
   - a private PS/2 backend consumed by the custom unified driver?
3. How should tuning controls be exposed long-term (C keycodes, Vial-specific path, or both)?

## Relationship to existing architecture doc

Use this strategy doc together with:

- `docs/holykeebs_pointing_architecture.md`

The architecture doc explains how HolyKeebs works today; this doc explains how to integrate equivalent behavior cleanly into this repository.

## Corne Rev1 implementation status

Current in-repo implementation is focused on `crkbd/rev1` with this topology:

- left half: Pimoroni trackball
- right half: PS/2 trackpoint
- master side: right
- OLED: disabled by default

Implemented integration paths:

- `users/holykeebs/*` userspace runtime and HK keycodes
- `drivers/sensors/ps2.[ch]` pointing-driver wrapper
- `drivers/ps2/ps2_mouse.[ch]` extended with `ps2_mouse_read()` and trackpoint register helpers
- `keyboards/crkbd/keymaps/hk/*` keymap + build config

## Corne Rev1 build and flash workflow

From repo root, build each half separately:

```bash
make crkbd/rev1:hk SIDE=right
cp .build/crkbd_rev1_hk.hex .build/crkbd_rev1_hk_right.hex

make crkbd/rev1:hk SIDE=left
cp .build/crkbd_rev1_hk.hex .build/crkbd_rev1_hk_left.hex
```

Flash the right-half firmware to the right controller and the left-half firmware to the left controller.

For convenience, you can also run:

```bash
./keyboards/crkbd/keymaps/hk/build_split.sh
```

That helper script performs both builds and writes:

- `.build/crkbd_rev1_hk_right.hex`
- `.build/crkbd_rev1_hk_left.hex`

### Wiring defaults used by the HK keymap

Default PS/2 pin config in `keyboards/crkbd/keymaps/hk/config.h`:

- `PS2_DATA_PIN D1`
- `PS2_CLOCK_PIN D0`

Adjust these if your trackpoint wiring differs.
