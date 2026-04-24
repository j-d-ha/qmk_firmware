# HolyKeebs Pointing Architecture (Developer + AI Notes)

This document captures how the HolyKeebs pointing stack is wired end-to-end, based on code inspection of:

- `/Users/jonasha/Repos/Other/holykeebs_qmk_firmware`

It is written as implementation documentation for future development, refactoring, and AI-assisted changes.

## Scope and assumptions

- This is about the HolyKeebs userspace architecture (build-time + runtime), not generic QMK pointing docs.
- The implementation analyzed lives in the external HolyKeebs firmware repo path above.
- This repository (`vial-hk-custom`) currently does not contain the HolyKeebs userspace files described here.

## 1) System model at a glance

The stack is userspace-centric and split-aware:

1. Build variables (`POINTING_DEVICE`, `POINTING_DEVICE_POSITION`, `SIDE`, `OLED`) are resolved in `users/holykeebs/rules.mk`.
2. Compile defines from rules are translated into low-level QMK config in `users/holykeebs/config.h`.
3. Runtime behavior is implemented in `users/holykeebs/holykeebs.c` using one state object (`g_hk_state`) plus EEPROM persistence.
4. Split builds optionally sync state over RPC (`users/holykeebs/rpc.c`) for mirrored display data.
5. UI is either HolyKeebs OLED (`users/holykeebs/oled.c`), stock OLED (board/keymap code), or Aztec42 LCD.

Core design intent: one logical "main pointer" abstraction regardless of physical half, with optional second pointer merged in combined mode.

## 2) Build-time control plane

Primary file: `users/holykeebs/rules.mk`

### Inputs

- `POINTING_DEVICE`: device topology selector (single or combined)
- `POINTING_DEVICE_POSITION`: where primary device is mounted (`left`, `right`, `middle`)
- `SIDE`: flashed half identity for heterogeneous dual-device splits (`left` or `right`)
- `OLED`: `yes`, `stock`, or unset
- `TRACKBALL_RGB_RAINBOW`: optional animation toggle

### Device allowlist

`POINTING_DEVICE` is validated against explicit values.

Single-device values include:

- `trackball`
- `trackpoint`
- `cirque35`
- `cirque40`
- `tps43`
- `tps65`

Combined values include mixed pairs (for example `trackball_tps43`, `trackpoint_cirque40`, plus symmetric forms).

### What rules.mk decides

- Enables pointer core (`POINTING_DEVICE_ENABLE = yes`) and common input features.
- Selects `POINTING_DEVICE_DRIVER` (`pimoroni_trackball`, `ps2`, `cirque_pinnacle_i2c`, `azoteq_iqs5xx`).
- Enables split-combined plumbing when needed (`SPLIT_POINTING_ENABLE`, `POINTING_DEVICE_COMBINED`).
- Emits side/type defines (`HK_POINTING_DEVICE_LEFT_*`, `HK_POINTING_DEVICE_RIGHT_*`, `HK_POINTING_DEVICE_MIDDLE_TPS65`).
- Emits master-side define (`HK_MASTER_LEFT` or `HK_MASTER_RIGHT`).
- For trackpoint paths: enables PS/2 vendor driver.
- Controls OLED routing:
  - `OLED=yes` -> HolyKeebs OLED (`HK_OLED_ENABLE` + `users/holykeebs/oled.c`)
  - `OLED=stock` -> stock OLED only
  - otherwise -> `OLED_ENABLE = no`

### Source inclusion behavior

When `POINTING_DEVICE` is set, userspace sources are added (including `holykeebs.c`, rpc/debug/device helper files). `oled.c` is only added for `OLED=yes`.

## 3) Compile-time translation layer

Primary file: `users/holykeebs/config.h`

`config.h` maps high-level HK defines to concrete QMK driver config and split behavior:

- Rewrites split-handedness policy from `HK_MASTER_LEFT` / `HK_MASTER_RIGHT`.
- Configures split serial/watchdog (`SERIAL_USART_TX_PIN GP1`, watchdog timeout 3000).
- Applies per-device orientation defines (especially Pimoroni and Azoteq cases).
- Applies mixed-device throttle workaround (`POINTING_DEVICE_TASK_THROTTLE_MS 1` in specific combos).
- Enables Cirque profile details (35/40 diameter, tap, gesture scroll).
- Enables Azoteq profile details (TPS43/TPS65, rotation, press-and-hold).
- Enables trackpoint PS/2 remote-mode pin configuration.
- Enables split state sync scaffolding only when `OLED_ENABLE && SPLIT_KEYBOARD`.

Important implication: many final runtime characteristics are emergent from this two-step config path (`rules.mk` -> `config.h`), not from keymap files.

## 4) Runtime state and data model

Primary files:

- `users/holykeebs/holykeebs.c`
- `users/holykeebs/pointing.h`
- `users/holykeebs/eeprom_config.h`
- `users/holykeebs/holykeebs.h`

### Global state

- `g_hk_state` is the runtime source of truth.
- It contains:
  - `main` pointer state
  - `peripheral` pointer state
  - display/telemetry fields (last key, last report, layer info)
  - dirty/sync flags

Pointer kinds are represented by enum in `pointing.h`:

- none
- pimoroni
- trackpoint
- cirque35
- cirque40
- tps43
- tps65

### Main/peripheral abstraction

- Runtime uses logical roles (`main`, `peripheral`), not fixed left/right identity.
- On split boards, initialization may swap roles on left-half builds so logical main follows intended topology.

### Defaults by pointer type

Defaults are assigned in `init_state` (unless overridden via compile macros):

- TPS43/TPS65: default 1.25, sniping 1.0, scroll buffer 5
- Pimoroni: default 1.5, sniping 1.0, scroll buffer 1
- Trackpoint: default 2.0, sniping 1.0, scroll buffer 5
- Cirque: default 1.0, sniping 1.0

### Persistence model

- EEPROM stores per-side tuning and mode data.
- Includes version/check fields (version currently 100) to validate layout compatibility.
- Stored values include cursor mode, drag-scroll, scroll lock, inversion, multipliers, and scroll buffer.

## 5) Runtime input/report pipeline

### Keycode-driven control API

Declared in `holykeebs.h` and handled in `process_record_user`.

Includes:

- save/reset/dump settings
- adjust default/sniping scaler
- adjust scroll buffer
- sniping mode (momentary/toggle)
- drag-scroll mode (momentary/toggle)
- cycle scroll lock
- invert scroll direction

Behavior detail:

- Shift modifies target side selection (acts on peripheral settings instead of main settings).
- Up/Down arrows change active setting values while in setting mode.

### Mouse report processing stages

Both single and combined paths route reports through the same processing concepts:

1. Multiplier scaling (mode-dependent)
2. Optional adaptive/drift hooks (compile-guarded)
3. Drag-scroll transform (`x/y` to `h/v`)
4. Inversion + scroll-lock rules
5. Buffered/stepped scroll output
6. HID-range clamping

Entry points:

- Single-device: `pointing_device_task_user(report)`
- Combined split: `pointing_device_task_combined_user(left, right)`

## 6) Split sync and RPC behavior

Primary files:

- `users/holykeebs/rpc.c`
- `users/holykeebs/rpc.h`

When split sync is enabled (through OLED + split conditions):

- Master transmits state periodically (about every 100 ms) when state is marked dirty.
- Peripheral receives and accepts only states flagged as main-side payloads.
- Peripheral stores received state with `is_main_side = false`.

Safety guard:

- `pointing.h` contains a static assert ensuring `hk_state_t` fits in `RPC_M2S_BUFFER_SIZE`.

## 7) Display integration modes

### HolyKeebs custom OLED

Primary file: `users/holykeebs/oled.c`

Displays key/pointer telemetry and pointer tuning state:

- pointer kind
- recent report values (`x/y/h/v`)
- mode and multipliers
- drag-scroll, scroll lock, scroll buffer
- key logger and layer info

### Stock OLED fallback

Board/keymap OLED code typically uses this guard pattern:

```c
#if defined(OLED_ENABLE) && !defined(HK_OLED_ENABLE)
```

Meaning:

- `OLED=stock` keeps board/keymap OLED implementations active.
- `OLED=yes` redirects to HolyKeebs OLED path.

### Aztec42 LCD path

Primary files:

- `keyboards/holykeebs/aztec42/lcd.c`
- `keyboards/holykeebs/aztec42/lcd.h`

This is a separate UI path (Quantum Painter + LVGL), currently rendering static label content and handling backlight timeout plus suspend/wake transitions.

## 8) Build orchestration script behavior

Primary file: `build_all.py`

Observed behavior:

- Injects `USER_NAME=holykeebs`.
- Builds many matrix combinations for split boards.
- Passes through variables like `POINTING_DEVICE`, `POINTING_DEVICE_POSITION`, `SIDE`, `OLED`, `CONSOLE`, and optional rainbow flag.
- Uses `hk` keymap for pointer builds and may choose `via` when pointer is absent.

This script is effectively the canonical test matrix for supported HolyKeebs pointing permutations.

## 9) Board/keymap integration footprint

HolyKeebs userspace headers are directly included by keymaps in:

- `keyboards/crkbd/keymaps/hk/keymap.c`
- `keyboards/crkbd/keymaps/idank/keymap.c`
- `keyboards/lily58/keymaps/hk/keymap.c`
- `keyboards/holykeebs/spankbd/keymaps/hk/keymap.c`
- `keyboards/holykeebs/sweeq/keymaps/hk/keymap.c`
- `keyboards/holykeebs/sweeq/keymaps/via/keymap.c`
- `keyboards/holykeebs/aztec42/keymaps/default/keymap.c`
- `keyboards/holykeebs/aztec42/keymaps/vial/keymap.c`

These keymaps often expose an admin/tuning layer with HK keycodes for runtime pointer tuning and persistence.

## 10) Known sharp edges and likely bugs

From implementation inspection:

1. `hk_get_dragscroll` return type appears mismatched (`hk_cursor_mode` vs boolean payload semantics).
2. Scroll buffer decrement path likely underflows (`uint8_t` check `>= 0` is always true).
3. Static accumulators in report processing likely bleed between main/peripheral paths in combined mode.
4. `rules.mk` OLED gating can unexpectedly disable OLED unless `OLED=yes|stock` is explicitly passed.
5. Some keymaps expose HK keycodes even when pointing userspace runtime is not linked (if `POINTING_DEVICE` unset).
6. `oled.c` pointer-kind formatter appears to miss explicit TPS65 string handling.
7. `OLED=yes` without `POINTING_DEVICE` appears unsafe because `oled.c` expects `g_hk_state` from `holykeebs.c`.

Treat these as backlog candidates before any major behavior refactor.

## 11) Practical extension guide (for developers and AI)

When adding a new pointing device or combo, update in this order:

1. `users/holykeebs/rules.mk`
   - extend allowlist
   - set driver/split flags
   - set HK side/type defines
2. `users/holykeebs/config.h`
   - add hardware/profile-specific config and orientation mapping
3. `users/holykeebs/pointing.h`
   - add pointer-kind enum value if needed
4. `users/holykeebs/holykeebs.c`
   - default multipliers/buffer
   - any mode-specific processing behavior
5. `users/holykeebs/oled.c`
   - pointer-kind string/display rendering
6. `build_all.py`
   - include new build variants in matrix

## 12) AI-agent operating checklist

For future AI edits against this stack:

- Confirm whether target behavior is build-time (`rules.mk`), compile-time (`config.h`), or runtime (`holykeebs.c`).
- Do not assume keymap `rules.mk` OLED flags are authoritative when HolyKeebs userspace rules are active.
- Preserve main/peripheral logical abstraction; do not hardcode left/right assumptions in runtime processing.
- Validate split safety when adding fields to `hk_state_t` (RPC payload size assert).
- Verify dual-device combined behavior for state isolation (avoid shared static accumulators across pointers).
- Ensure display code gracefully handles all pointer-kind enum values.

## 13) Suggested verification matrix

Minimum functional matrix after touching pointer logic:

- Single trackball (`POINTING_DEVICE=trackball`)
- Single trackpoint (`POINTING_DEVICE=trackpoint`)
- Single cirque (`cirque35` and/or `cirque40`)
- Single azoteq (`tps43` and/or `tps65`)
- Mixed combined mode (for example `trackball_tps43` or `trackpoint_cirque40`) on both flashed halves (`SIDE=left`, `SIDE=right`)
- OLED mode checks: `OLED=yes` and `OLED=stock`
- EEPROM upgrade path: invalid/old config initialization and save/load behavior

## 14) File index (analyzed implementation)

- `/Users/jonasha/Repos/Other/holykeebs_qmk_firmware/users/holykeebs/rules.mk`
- `/Users/jonasha/Repos/Other/holykeebs_qmk_firmware/users/holykeebs/config.h`
- `/Users/jonasha/Repos/Other/holykeebs_qmk_firmware/users/holykeebs/holykeebs.c`
- `/Users/jonasha/Repos/Other/holykeebs_qmk_firmware/users/holykeebs/holykeebs.h`
- `/Users/jonasha/Repos/Other/holykeebs_qmk_firmware/users/holykeebs/pointing.h`
- `/Users/jonasha/Repos/Other/holykeebs_qmk_firmware/users/holykeebs/eeprom_config.h`
- `/Users/jonasha/Repos/Other/holykeebs_qmk_firmware/users/holykeebs/rpc.c`
- `/Users/jonasha/Repos/Other/holykeebs_qmk_firmware/users/holykeebs/rpc.h`
- `/Users/jonasha/Repos/Other/holykeebs_qmk_firmware/users/holykeebs/oled.c`
- `/Users/jonasha/Repos/Other/holykeebs_qmk_firmware/users/holykeebs/pimoroni.c`
- `/Users/jonasha/Repos/Other/holykeebs_qmk_firmware/users/holykeebs/trackpoint.c`
- `/Users/jonasha/Repos/Other/holykeebs_qmk_firmware/build_all.py`

---

If this stack is migrated into this repo later, keep this document and add in-repo relative paths alongside the external references.
