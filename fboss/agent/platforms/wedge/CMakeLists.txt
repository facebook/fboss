
include(fboss/agent/hw/bcm/CMakeLists.txt)

target_sources(fboss_agent
  PRIVATE
    # platform specific files
    fboss/agent/platforms/wedge/oss/GalaxyPlatform.cpp
    fboss/agent/platforms/wedge/oss/GalaxyPort.cpp
    fboss/agent/platforms/wedge/oss/WedgePlatform.cpp
    fboss/agent/platforms/wedge/oss/Wedge40Platform.cpp
    fboss/agent/platforms/wedge/oss/Wedge100Platform.cpp
    fboss/agent/platforms/wedge/oss/WedgePort.cpp
    fboss/agent/platforms/wedge/oss/Wedge40Port.cpp
    fboss/agent/platforms/wedge/oss/Wedge100Port.cpp
    fboss/agent/platforms/wedge/oss/WedgePlatformInit.cpp
    fboss/agent/platforms/wedge/oss/WedgeProductInfo.cpp
    fboss/agent/platforms/wedge/wedge_agent.cpp
    fboss/agent/platforms/wedge/WedgePlatform.cpp
    fboss/agent/platforms/wedge/Wedge40Platform.cpp
    fboss/agent/platforms/wedge/Wedge100Platform.cpp
    fboss/agent/platforms/wedge/GalaxyLCPlatform.cpp
    fboss/agent/platforms/wedge/GalaxyFCPlatform.cpp
    fboss/agent/platforms/wedge/WedgePort.cpp
    fboss/agent/platforms/wedge/Wedge100Port.cpp
    fboss/agent/platforms/wedge/WedgeProductInfo.cpp
    fboss/agent/platforms/wedge/WedgePlatformInit.cpp
)

add_executable(wedge_agent
  fboss/agent/platforms/wedge/wedge_agent.cpp
)

target_link_libraries(wedge_agent
                      fboss_agent
)

