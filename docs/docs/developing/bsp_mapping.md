# FBOSS BSP Mapping

A platform's BSP mapping is derived directly from its `platform_manager.json` (`fboss/platform/configs/PLATFORM/platform_manager.json`).

The same configuration that PlatformManager uses to create the transceiver and LED device nodes is also the single source of the BSP mapping consumed by `qsfp_service` and `led_service`. There is no separate BSP mapping input to maintain.

For example, the `lanesPerPort` field on a port's LED control block, together with `ledPerPort`, lets the lane-to-LED mapping be derived directly from `platform_manager.json`: the port's lanes are distributed evenly across its LEDs, so a port with `lanesPerPort: 8` and `ledPerPort: 2` maps lanes 1–4 to the first LED and lanes 5–8 to the second.

## Transceiver access paths

For each transceiver `N`, PlatformManager creates a consistent set of device nodes under `/run/devmap/xcvrs/`, all keyed off the transceiver id — so transceiver `N` always uses index `N`, with no per-platform translation table:

- `xcvr_io_N` — I2C data path to the module; `qsfp_service` uses it to read/write the module memory map (EEPROM/ID, DOM, registers, config).
- `xcvr_ctrl_N` — control node exposing `xcvr_reset_N` (reset) and `xcvr_present_N` (presence detect).
