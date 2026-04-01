---
id: service_api
title: Thrift Service API
sidebar_label: Service API
sidebar_position: 9
---

# PlatformManager Service API

PlatformManager exposes a Thrift service on port 5975 (default) that allows
querying platform state and exploration status.

## Service Endpoints

The Thrift interface is defined in
[platform_manager_service.thrift](https://github.com/facebook/fboss/blob/main/fboss/platform/platform_manager/platform_manager_service.thrift).

### getPlatformName()

Returns the platform name as detected from dmidecode.

**Response**: `string`

### getAllPmUnits()

Returns information about all discovered PmUnits in the platform.

**Response**: `PmUnitsResponse`
- `pmUnits`: Map from slot path to `PmUnitInfo`

### getLastPMStatus()

Returns the status of the most recent platform exploration.

**Response**: `PlatformManagerStatus`
- `explorationStatus`: One of:
  - `SUCCEEDED`: All devices explored successfully
  - `IN_PROGRESS`: Exploration currently running
  - `FAILED`: Exploration encountered errors
  - `SUCCEEDED_WITH_EXPECTED_ERRORS`: Completed with expected errors (e.g., empty slots)
  - `UNSTARTED`: Exploration has not started
- `lastExplorationTime`: Unix timestamp of last exploration completion
- `failedDevices`: Map of device paths to their exploration errors

### getPmUnitInfo(slotPath)

Returns information about a specific PmUnit at the given slot path.


**Response**: `PmUnitInfoResponse`
- `pmUnitInfo`: Contains name, version, presence info, and exploration status


### getBspVersion()

Returns BSP package version information.

**Response**: `BspVersionResponse`
- `bspBaseName`: Base name of the BSP RPM
- `bspVersion`: Version string of the installed BSP
- `kernelVersion`: Running kernel version


### getEepromContents(slotPath)

Returns the EEPROM contents for the PmUnit at the given slot path.


## Client Usage

The PlatformManager service is consumed programmatically by other FBOSS services.
For example, `sensor_service` uses `PmClientFactory` to connect. The following is an example of using a custom thrift client to query the service.

```
./pm_thrift_client getPlatformName
MONTBLANC


./pm_thrift_client getBspVersion
{
  "bspBaseName": "fboss_bsp_kmods",
  "bspVersion": "3.4.0-1",
  "kernelVersion": "6.12.49-1.el9.x86_64"
}
```
