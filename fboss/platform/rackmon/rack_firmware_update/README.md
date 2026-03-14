# Rack Firmware Update Tools

This directory contains firmware update tools for rack-mounted power devices (PSUs and BBUs) that communicate via Modbus/RTU through the rackmon service.

## Overview

Two firmware update tools are provided:

1. **psu_update_delta_orv3** - Updates Delta ORv3 PSUs using MEI protocol
2. **device_update_mailbox** - Updates BBU and other devices using Mailbox protocol

Both tools communicate with devices through the rackmon Thrift service, which manages the Modbus/RTU communication.

## Tools

### psu_update_delta_orv3

Updates firmware on Delta ORv3 PSUs using the MEI (Modbus Encapsulated Interface) protocol.

**Supported Devices:**
- Delta ORv3 PSUs (DPST3000GBA and similar models)

**Protocol:** MEI (Modbus function code 0x2B)

**Usage:**
```bash
# List all discovered devices
psu_update_delta_orv3 --list-devices

# Update PSU firmware
psu_update_delta_orv3 \
  --addr <device_address> \
  --key <security_key> \
  --firmware <firmware_file>
```

**Example:**
```bash
# Update PSU at address 232 (0xe8)
psu_update_delta_orv3 \
  --addr 232 \
  --key 0x06854137C758A5B6 \
  --firmware V3_DFLASH_REV_01_00_07_00.dflash
```

**Firmware File Format:** `.dflash` or `.hex` (Intel HEX format)

### device_update_mailbox

Updates firmware on BBU (Battery Backup Unit) and other devices using a mailbox-based protocol.

**Supported Vendors:**
- Panasonic
- Delta
- AEI
- HPR (High Power Rectifier) variants

**Protocol:** Mailbox (standard Modbus registers)

**Usage:**
```bash
# List all discovered devices
device_update_mailbox --list-devices

# Update device firmware
device_update_mailbox \
  --addr <device_address> \
  --vendor <vendor_name> \
  --firmware <firmware_file>
```

**Example:**
```bash
# Update Delta BBU at address 64 (0x40)
device_update_mailbox \
  --addr 64 \
  --vendor delta \
  --firmware DPST3000GBA_IMG_APP_FB_S10604_S10600.bin
```

**Supported Vendors:**
- `panasonic` - Panasonic BBU devices
- `delta` - Delta BBU devices
- `delta_power_tether` - Delta devices with power tether
- `aei` - AEI devices
- `hpr_v1`, `hpr_v2`, `hpr_v3` - HPR variants

**Firmware File Format:** `.bin` (binary firmware image)

## Building

Build both tools using Buck:

```bash
# Build PSU update tool
buck build @mode/opt --show-output //fboss/platform/rackmon/rack_firmware_update:psu_update_delta_orv3

# Build BBU update tool
buck build @mode/opt --show-output //fboss/platform/rackmon/rack_firmware_update:device_update_mailbox

# Build both
buck build @mode/opt --show-output //fboss/platform/rackmon/rack_firmware_update:psu_update_delta_orv3 \
            //fboss/platform/rackmon/rack_firmware_update:device_update_mailbox
```

Binaries will be located at:
- `buck-out/v2/gen/fbcode/.../psu_update_delta_orv3`
- `buck-out/v2/gen/fbcode/.../device_update_mailbox`

## Prerequisites

### Rackmon Service

Both tools require the rackmon service to be running:

```bash
# Check service status
systemctl status rackmon

# Start service if needed
sudo systemctl start rackmon

# Enable service to start on boot
sudo systemctl enable rackmon
```

### Device Discovery

Before updating, verify the device is detected:

```bash
# List all devices
psu_update_delta_orv3 --list-devices
# or
device_update_mailbox --list-devices
```

Expected output:
```
Discovered Modbus Devices:
============================================================
Address:  64 (0x40)  Type: 3  Mode: DORMANT
Address:  65 (0x41)  Type: 3  Mode: DORMANT
Address: 232 (0xe8)  Type: 1  Mode: DORMANT
============================================================
Total: 3 device(s)
```

## Firmware Update Process

Both tools follow a similar high-level process:

1. **Validate Device**
   - Confirms device exists at specified address
   - Verifies device is accessible

2. **Pause Monitoring**
   - Temporarily stops rackmon's periodic device polling
   - Prevents interference with firmware update

3. **Update Firmware**
   - Authenticates (MEI) or unlocks (Mailbox)
   - Transfers firmware blocks
   - Verifies firmware integrity
   - Finalizes update

4. **Resume Monitoring**
   - Restarts rackmon's periodic device polling
   - Returns to normal operation

## Protocol Details

### MEI Protocol (psu_update_delta_orv3)

The MEI protocol uses Modbus function code 0x2B with vendor-specific commands:

**Update Steps:**
1. Get Seed (0x01) - Device sends random 8-byte seed
2. Send Key (0x02) - Host sends XORed seed+key for authentication
3. Erase Flash (0x03) - Erases target flash memory
4. Write Block (0x04) - Transfers 128-byte firmware blocks
5. Verify Firmware (0x05) - Device verifies CRC
6. Finalize (0x06) - Commits new firmware

**Security:**
- Requires 64-bit security key for authentication
- Key is XORed with device-provided seed
- Authentication failure aborts update

### Mailbox Protocol (device_update_mailbox)

The mailbox protocol uses standard Modbus registers:

**Update Steps:**
1. Unlock Engineering Mode - Enters bootloader
2. Enter Boot Mode - Prepares for firmware transfer
3. Transfer Firmware - Writes blocks to data registers
4. Verify Firmware - Device calculates and checks CRC
5. Exit Boot Mode - Reboots with new firmware

**Vendor Differences:**
- Block size: 64 bytes (Panasonic/Delta) or 128 bytes (AEI)
- Register addresses vary by vendor
- Some vendors require special workarounds

## Error Handling

Both tools implement robust error handling:

- **Automatic Retries:** Failed operations retry up to 5 times
- **CRC Validation:** All Modbus frames include CRC checks
- **Timeout Handling:** Configurable timeouts for slow operations
- **Progress Display:** Real-time progress updates during transfer
- **Graceful Cleanup:** Monitoring always resumed, even on error

Common errors and solutions:

| Error                                | Cause                                | Solution                                  |
|--------------------------------------|--------------------------------------|-------------------------------------------|
| Load Shedding Due to Queue Timeout  | Rackmon service overloaded           | Manually restart: sudo systemctl restart rackmon |
| Device not found                     | Device offline or wrong address      | Check device with --list-devices          |
| Modbus I/O failure                   | Communication error                  | Check cables, retry update                |
| Authentication failed                | Wrong security key                   | Verify correct key for device             |
| Verification failed                  | Corrupt firmware or transfer error   | Retry with valid firmware file            |

## Architecture

### Code Organization

```
rack_firmware_update/
├── BUCK                          # Build configuration
├── README.md                     # This file
│
├── psu_update_delta_orv3.cpp     # PSU update tool (MEI protocol)
├── device_update_mailbox.cpp     # BBU update tool (Mailbox protocol)
│
├── FirmwareUpdater.h/cpp         # Base class for firmware updaters
├── MEIFirmwareUpdater.h/cpp      # MEI protocol implementation
├── MailboxFirmwareUpdater.h/cpp  # Mailbox protocol implementation
│
├── RackmonClient.h/cpp           # Thrift client wrapper
├── UpdaterUtils.h/cpp            # Utility functions
├── UpdaterExceptions.h/cpp       # Exception hierarchy
└── HexFileParser.h/cpp           # Intel HEX file parser
```

### Class Hierarchy

```
FirmwareUpdater (base class)
├── MEIFirmwareUpdater (MEI protocol)
└── MailboxFirmwareUpdater (Mailbox protocol)

RackmonClient (Thrift wrapper)
├── Persistent connection management
├── Modbus operations (read/write registers)
├── Raw command support
└── Monitoring control (pause/resume)
```

### Key Components

**RackmonClient:**
- Wraps rackmon Thrift service
- Maintains persistent connection
- Provides clean C++ API for Modbus operations
- Handles byte conversions and error checking

**FirmwareUpdater:**
- Base class for all firmware updaters
- Manages RackmonClient instance
- Provides common device operations
- Handles monitoring pause/resume

**UpdaterUtils:**
- Device scanning and validation
- Retry logic with exponential backoff
- PMM (Power Management Module) control

**UpdaterExceptions:**
- Exception hierarchy for error handling
- Specific exceptions for different error types
- Modbus-specific exceptions (CRC, timeout, I/O)

## Testing

### Unit Tests

Run rackmon unit tests to verify core functionality:

```bash
buck test fboss/platform/rackmon:rackmon_test
```

### Integration Testing

Test on actual hardware:

1. **List Devices:**
   ```bash
   ./psu_update_delta_orv3 --list-devices
   ./device_update_mailbox --list-devices
   ```

2. **Dry Run (read version only):**
   ```bash
   # PSU tool reads version during validation
   ./psu_update_delta_orv3 --addr 232 --key 0x06854137C758A5B6 --firmware test.dflash
   # (Ctrl+C after version is displayed)
   ```

3. **Full Update:**
   ```bash
   # Update with valid firmware file
   ./psu_update_delta_orv3 --addr 232 --key 0x06854137C758A5B6 --firmware valid.dflash
   ```

4. **Verify Update:**
   ```bash
   # Check new version after update
   ./psu_update_delta_orv3 --list-devices
   ```

## Troubleshooting

### Service Issues

**Problem:** "Load Shedding Due to Queue Timeout"
```bash
# Solution: Manually restart rackmon service if needed
sudo systemctl restart rackmon
```

**Problem:** "Device not found"
```bash
# Check service is running
systemctl status rackmon

# Check device list
./psu_update_delta_orv3 --list-devices

# Verify device address
# PSUs are typically 0xe8 (232)
# BBUs are typically 0x40-0x4f (64-79)
```

### Communication Issues

**Problem:** "Modbus I/O failure"
```bash
# Check physical connections
# Verify device is powered on
# Check Modbus bus is not overloaded
# Retry update
```

**Problem:** "Modbus operation timed out"
```bash
# Device may be busy
# Wait a few seconds and retry
# Check if another process is accessing device
```

### Firmware Issues

**Problem:** "Authentication failed" (MEI protocol)
```bash
# Verify security key is correct
# Key format: 0x1234567890ABCDEF (16 hex digits)
# Contact vendor for correct key
```

**Problem:** "Verification failed"
```bash
# Firmware file may be corrupt
# Download firmware file again
# Verify firmware is for correct device model
# Check file integrity (MD5/SHA256)
```

## Safety and Best Practices

### Before Updating

1. ✅ **Verify firmware file** - Ensure firmware is for correct device model
2. ✅ **Backup configuration** - Save device configuration if applicable
3. ✅ **Check power** - Ensure stable power supply during update
4. ✅ **Plan downtime** - Update takes 5-10 minutes, device will be offline
5. ✅ **Test on non-production** - Test on development device first

### During Update

1. ⚠️ **Do not interrupt** - Do not stop the update process
2. ⚠️ **Do not power cycle** - Do not reboot device during update
3. ⚠️ **Monitor progress** - Watch for errors or unexpected behavior
4. ⚠️ **Keep logs** - Save output for troubleshooting if needed

### After Update

1. ✅ **Verify version** - Confirm new firmware version is correct
2. ✅ **Test functionality** - Verify device operates correctly
3. ✅ **Monitor stability** - Watch for issues over next 24 hours
4. ✅ **Document update** - Record firmware version and update date

## Performance

Typical firmware update times:

| Device Type          | Firmware Size | Transfer Time | Verification Time | Total Time   |
|----------------------|---------------|---------------|-------------------|--------------|
| Delta PSU (MEI)      | ~300 KB       | 3-5 minutes   | 10-30 seconds     | 4-6 minutes  |
| Delta BBU (Mailbox)  | ~500 KB       | 5-8 minutes   | 60-90 seconds     | 7-10 minutes |
| Panasonic BBU        | ~400 KB       | 4-6 minutes   | 60-90 seconds     | 6-8 minutes  |

Factors affecting update time:
- Modbus baud rate (typically 19200 or 38400)
- Firmware size
- Device flash write speed
- Modbus bus congestion

## Logging

Both tools output progress to stdout and errors to stderr:

**Normal output:**
```
============================================================
Step 1: Validate Device
============================================================
Device found at address 0xe8
Current version: 31502058

============================================================
Step 2: Firmware Update
============================================================
Pausing monitoring...
Parsing Firmware
Send get seed
Send key
Erase flash
Transferring firmware...
[10.00%] Sending block 100 of 1000...
[20.00%] Sending block 200 of 1000...
...
[100.00%] Sending block 1000 of 1000...
Verify firmware
Finalize update
Resuming monitoring...
Firmware update completed successfully!
```

**Error output:**
```
Error: Device not found at address 0xe8
Error: Authentication failed: Invalid security key
Error: Firmware update failed: Modbus I/O failure
```

## Contributing

When modifying these tools:

1. **Maintain backward compatibility** - Don't break existing functionality
2. **Add tests** - Update unit tests for new features
3. **Document changes** - Update this README and code comments
4. **Follow style guide** - Use clang-format for C++ code
5. **Test thoroughly** - Test on actual hardware before committing

## References

- **Modbus Protocol:** https://modbus.org/docs/Modbus_Application_Protocol_V1_1b3.pdf
- **MEI Protocol:** Modbus function code 0x2B (Encapsulated Interface Transport)
- **Rackmon Service:** fboss/platform/rackmon/README.md
- **Thrift Interface:** fboss/platform/rackmon/if/rackmonsvc.thrift

## Support

For issues or questions:

1. Check this README and tool documentation
2. Review error messages and troubleshooting section
3. Check rackmon service logs: `journalctl -u rackmon -f`
4. Contact FBOSS Platform team (oncall: fboss_platform)

## License

Copyright (c) 2004-present, Facebook, Inc.
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
