# New Platform Support

## Overview

This document describes all of the locations in the code where you need to add code to onboard a new platform in qsfp_service / agent code. Please follow this [GitHub Commit](https://github.com/facebook/fboss/commit/00b86dfee299bd164f6380938c501e0ee92482c2) as an example if the code pointers are not sufficient.

### Common Code Changes
1. Add a new entry to the PlatformType enum in [fboss/lib/if/fboss_common.thrift](https://github.com/facebook/fboss/blob/main/fboss/lib/if/fboss_common.thrift).
2. Add a new entry to the `toString(PlatformType mode)` function in [fboss/lib/platforms/PlatformMode.h](https://github.com/facebook/fboss/blob/main/fboss/lib/platforms/PlatformMode.h).
3. Add a new entry to `initMode()` in [fboss/lib/platforms/PlatformProductInfo.cpp](https://github.com/facebook/fboss/blob/main/fboss/lib/platforms/PlatformProductInfo.cpp).
4. Add a new switch case for your platform to `initPlatformMapping()` in [fboss/agent/platforms/common/PlatformMappingUtils.cpp](https://github.com/facebook/fboss/blob/main/fboss/agent/platforms/common/PlatformMappingUtils.cpp#L124).
6. Based on your files created in [Platform Mapping Config Generation](https://facebook.github.io/fboss/docs/developing/platform_mapping/), create a new folder and add all platform mapping CSV files to [fboss/lib/platform_mapping_v2/platforms/{PLATFORM}/](https://github.com/facebook/fboss/tree/main/fboss/lib/platform_mapping_v2/platforms).
7. Similarly, create a new folder and add a source file and header for your platform mapping. This requires copying the generated platform mapping JSON into `kJsonPlatformMappingStr` within the `.cpp` file.
   - `fboss/agent/platforms/common/{PLATFORM}/{PLATFORM}PlatformMapping.h` [Header example](https://github.com/facebook/fboss/blob/main/fboss/agent/platforms/common/montblanc/MontblancPlatformMapping.h)
   - `fboss/agent/platforms/common/{PLATFORM}/{PLATFORM}PlatformMapping.cpp` [Source example](https://github.com/facebook/fboss/blob/main/fboss/agent/platforms/common/montblanc/MontblancPlatformMapping.cpp)

### Agent Code Changes
1. Add a `SaiBcm{PLATFORM}PlatformPort` header and source file for your platform:
   - `fboss/agent/platforms/sai/SaiBcm{PLATFORM}PlatformPort.h` [Header example](https://github.com/facebook/fboss/blob/ad0121584f3dc5f9b3d631c2473d14412599e6c7/fboss/agent/platforms/sai/SaiBcmMontblancPlatformPort.h)
   - `fboss/agent/platforms/sai/SaiBcm{PLATFORM}PlatformPort.cpp` [Source example](https://github.com/facebook/fboss/blob/ad0121584f3dc5f9b3d631c2473d14412599e6c7/fboss/agent/platforms/sai/SaiBcmMontblancPlatformPort.cpp)

2. Add a `SaiBcm{PLATFORM}Platform` header and source file for your platform:
   - `fboss/agent/platforms/sai/SaiBcm{PLATFORM}Platform.h` [Header example](https://github.com/facebook/fboss/blob/ad0121584f3dc5f9b3d631c2473d14412599e6c7/fboss/agent/platforms/sai/SaiBcmMontblancPlatform.h)
   - `fboss/agent/platforms/sai/SaiBcm{PLATFORM}Platform.cpp` [Source example](https://github.com/facebook/fboss/blob/ad0121584f3dc5f9b3d631c2473d14412599e6c7/fboss/agent/platforms/sai/SaiBcmMontblancPlatform.cpp)

3. Add a new entry to `initPorts()` in [fboss/agent/platforms/sai/SaiPlatform.cpp](https://github.com/facebook/fboss/blob/main/fboss/agent/platforms/sai/SaiPlatform.cpp).
4. Add a new entry to `chooseSaiPlatform()` in [fboss/agent/platforms/sai/SaiPlatformInit.cpp](https://github.com/facebook/fboss/blob/main/fboss/agent/platforms/sai/SaiPlatformInit.cpp).
5. Make changes to CMake files to support building in open source:
   - Add `cmake/AgentPlatformsCommon{PLATFORM}.cmake` for your platform ([Example](https://github.com/facebook/fboss/blob/main/cmake/AgentPlatformsCommonMontblanc.cmake))
   - Then add this library name to `sai_platform` link libraries in [cmake/AgentPlatformsSai.cmake](https://github.com/facebook/fboss/blob/main/cmake/AgentPlatformsSai.cmake#L104)

### Qsfp Service Code Changes
1. Create BspPlatformMapping header and source files under
   `fboss/lib/bsp/{PLATFORM}/`
   - `fboss/lib/bsp/{PLATFORM}/{PLATFORM}BspPlatformMapping.h` [Header example](https://github.com/facebook/fboss/blob/main/fboss/lib/bsp/montblanc/MontblancBspPlatformMapping.h)
   - `fboss/lib/bsp/{PLATFORM}/{PLATFORM}BspPlatformMapping.cpp` [Source example](https://github.com/facebook/fboss/blob/main/fboss/lib/bsp/montblanc/MontblancBspPlatformMapping.cpp)

2. Add an LED Manager class for your platform:
   - `fboss/led_service/{PLATFORM}LedManager.h` [Header example](https://github.com/facebook/fboss/blob/main/fboss/led_service/MontblancLedManager.h)
   - `fboss/led_service/{PLATFORM}LedManager.cpp` [Source example](https://github.com/facebook/fboss/blob/main/fboss/led_service/MontblancLedManager.cpp)
   - Add to [fboss/led_service/LedManagerInit.cpp](https://github.com/facebook/fboss/blob/main/fboss/led_service/LedManagerInit.cpp).

3. In `fboss/qsfp_service/platforms/wedge/WedgeManagerInit.cpp`, add a function called `create{PLATFORM}WedgeManager` that instantiates a `WedgeManager` object with the platform mapping JSON file.
   - `fboss/qsfp_service/platforms/wedge/WedgeManagerInit.h` [Header example](https://github.com/facebook/fboss/blob/main/fboss/qsfp_service/platforms/wedge/WedgeManagerInit.h#L59)
   - `fboss/qsfp_service/platforms/wedge/WedgeManagerInit.cpp` [Source example](https://github.com/facebook/fboss/blob/main/fboss/qsfp_service/platforms/wedge/WedgeManagerInit.cpp#L181)

4. Make changes to CMake files to support building in open source:
   - Add BSP library definition to [cmake/QsfpService.cmake](https://github.com/facebook/fboss/blob/main/cmake/QsfpService.cmake) and link to `qsfp_bsp_core`
   - Add LED Manager source file to `led_manager_lib` in [cmake/LedService.cmake](https://github.com/facebook/fboss/blob/main/cmake/LedService.cmake) and link BSP and platform mapping libraries
   - Add platform mapping library to `qsfp_platforms_wedge` in [cmake/QsfpServicePlatformsWedge.cmake](https://github.com/facebook/fboss/blob/main/cmake/QsfpServicePlatformsWedge.cmake)
