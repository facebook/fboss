---
id: presence_detection
title: Presence Detection
sidebar_label: Presence Detection
sidebar_position: 4
---

# PlatformManager Presence Detection

PlatformManager supports detecting whether a PmUnit (FRU) is physically present
in a slot. This enables hot-plug detection and proper handling of optional
components.

## Overview

Presence detection determines if a PmUnit is plugged into a slot before
attempting to explore it. This prevents errors when accessing empty slots and
enables reporting of which slots are populated.

## Detection Methods

### GPIO Line Handle

Uses a GPIO line from a gpio chip to detect presence:

```json
{
  "presenceDetection": {
    "gpioLineHandle": {
      "devicePath": "/[SCM_CPLD_GPIO]",
      "lineIndex": 5,
      "desiredValue": 1
    }
  }
}
```

- `devicePath`: DevicePath to the gpio chip device
- `lineIndex`: GPIO line number (0-indexed)
- `desiredValue`: Value indicating presence (typically 0 or 1)

### Sysfs File Handle

Uses a sysfs file to detect presence:

```json
{
  "presenceDetection": {
    "sysfsFileHandle": {
      "devicePath": "/[SMB_CPLD]",
      "presenceFileName": "pim1_present",
      "desiredValue": 1
    }
  }
}
```

- `devicePath`: DevicePath to the device exposing the sysfs file
- `presenceFileName`: Name of the sysfs file containing presence status
- `desiredValue`: Value indicating presence

## Configuration

Presence detection is configured per-slot in `SlotConfig`:

```json
{
  "outgoingSlotConfigs": {
    "PIM_SLOT@0": {
      "slotType": "PIM_SLOT",
      "presenceDetection": {
        "gpioLineHandle": {
          "devicePath": "/[IOB_GPIO]",
          "lineIndex": 0,
          "desiredValue": 1
        }
      },
      "outgoingI2cBusNames": ["IOB_I2C_0@0"]
    }
  }
}
```

## Presence Info Response

When querying PmUnit info via the Thrift API, presence information is included:

```
PmUnitInfo {
  name: "PIM8DD",
  presenceInfo: {
    presenceDetection: { ... },
    actualValue: 1,
    isPresent: true
  },
  successfullyExplored: true
}
```

- `actualValue`: The raw value read from the GPIO or sysfs file
- `isPresent`: Whether the actual value matches the desired value

## Exploration Behavior

1. If `presenceDetection` field is configured for a slot:
   - PlatformManager checks presence before exploring
   - If FRU is absent: the slot is skipped and a `SLOT_PM_UNIT_ABSENCE` error is
     recorded
   - If FRU is present: normal exploration proceeds

2. If `presenceDetection` field is NOT configured:
   - The slot is assumed to always be populated
   - Exploration proceeds directly

## Expected Empty Slots

Some slots may be expected to be empty. These are tracked separately from
unexpected failures. When a slot is expected to be empty, its absence is
reported as `SUCCEEDED_WITH_EXPECTED_ERRORS` rather than `FAILED`. Currently, only `PSU_SLOT` types are treated as expected-to-be-empty.

## Hardware Requirements

For GPIO-based presence detection:
- The GPIO chip must be created before checking presence
- Parent PmUnit must be explored first

For sysfs-based presence detection:
- The device exposing the sysfs file must be created first
- The sysfs file must be readable
