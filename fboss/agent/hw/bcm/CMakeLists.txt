add_thrift_cpp2_library(
  packettrace_cpp2
  fboss/agent/hw/bcm/packettrace.thrift
)
add_thrift_cpp2_library(
  bcmswitch_cpp2
  fboss/agent/hw/bcm/bcmswitch.thrift
)
add_thrift_cpp2_library(
  hardware_stats_cpp2
  fboss/agent/hw/bcm/hardware_stats.thrift
)

# opennsl for BCM-based switches
find_library(OPENNSL opennsl
             PATHS ${CMAKE_SOURCE_DIR}/external/OpenNSL/bin/wedge-trident)

target_include_directories(fboss_agent
  PRIVATE
    ${CMAKE_SOURCE_DIR}/external/OpenNSL/include
)

target_sources(fboss_agent
  PRIVATE
    # ASIC specific files
    fboss/agent/hw/bcm/BcmAclRange.cpp
    fboss/agent/hw/bcm/BcmAclTable.cpp
    fboss/agent/hw/bcm/BcmAPI.cpp
    fboss/agent/hw/bcm/BcmEgress.cpp
    fboss/agent/hw/bcm/BcmHost.cpp
    fboss/agent/hw/bcm/BcmHostKey.cpp
    fboss/agent/hw/bcm/BcmIntf.cpp
    fboss/agent/hw/bcm/BcmPlatform.cpp
    fboss/agent/hw/bcm/BcmPort.cpp
    fboss/agent/hw/bcm/BcmPortGroup.cpp
    fboss/agent/hw/bcm/BcmPortQueueManager.cpp
    fboss/agent/hw/bcm/BcmPortTable.cpp
    fboss/agent/hw/bcm/BcmRoute.cpp
    fboss/agent/hw/bcm/BcmRxPacket.cpp
    fboss/agent/hw/bcm/BcmSflowExporter.cpp
    fboss/agent/hw/bcm/BcmStats.cpp
    fboss/agent/hw/bcm/BcmSwitch.cpp
    fboss/agent/hw/bcm/BcmSwitchEventCallback.cpp
    fboss/agent/hw/bcm/BcmSwitchEventUtils.cpp
    fboss/agent/hw/bcm/BcmTrunkStats.cpp
    fboss/agent/hw/bcm/BcmTrunkTable.cpp
    fboss/agent/hw/bcm/BcmTxPacket.cpp
    fboss/agent/hw/bcm/BcmWarmBootCache.cpp
    fboss/agent/hw/bcm/BcmWarmBootHelper.cpp
    fboss/agent/hw/bcm/PortAndEgressIdsMap.cpp
    fboss/agent/hw/bcm/oss/BcmAclEntry.cpp
    fboss/agent/hw/bcm/oss/BcmAclRange.cpp
    fboss/agent/hw/bcm/oss/BcmAclStat.cpp
    fboss/agent/hw/bcm/oss/BcmAPI.cpp
    fboss/agent/hw/bcm/oss/BcmControlPlane.cpp
    fboss/agent/hw/bcm/oss/BcmEgress.cpp
    fboss/agent/hw/bcm/oss/BcmHost.cpp
    fboss/agent/hw/bcm/oss/BcmPort.cpp
    fboss/agent/hw/bcm/oss/BcmPortGroup.cpp
    fboss/agent/hw/bcm/oss/BcmPortQueueManager.cpp
    fboss/agent/hw/bcm/oss/BcmPortTable.cpp
    fboss/agent/hw/bcm/oss/BcmSwitch.cpp
    fboss/agent/hw/bcm/oss/BcmSwitchEventCallback.cpp
    fboss/agent/hw/bcm/oss/BcmTrunk.cpp
    fboss/agent/hw/bcm/oss/BcmUnit.cpp
    fboss/agent/hw/bcm/oss/BcmWarmBootCache.cpp
    fboss/agent/hw/bcm/oss/BcmWarmBootHelper.cpp
    fboss/agent/hw/bcm/oss/BcmTableStats.cpp
    fboss/agent/hw/bcm/Utils.cpp
)

target_link_libraries(fboss_agent PRIVATE
    packettrace_cpp2
    bcmswitch_cpp2
    hardware_stats_cpp2
    ${OPENNSL}
)

