cmake_minimum_required(VERSION 2.8)
if (POLICY CMP0054)
  # Enable CMP0054 to indicate that we want the new safer if() argument behavior
  cmake_policy(SET CMP0054 NEW)
endif()
project(FBOSS)

include(CheckCXXCompilerFlag)
include(CMakeParseArguments)

# Check the version
CHECK_CXX_COMPILER_FLAG("-std=gnu++1y" COMPILER_SUPPORTS_CXX14)
if(NOT COMPILER_SUPPORTS_CXX14)
  message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++14 support.")
endif()

# Ideally, we would use
#    set_property(TARGET fboss_agent PROPERTY CXX_STANDARD 11)
# to define the C++ version, but we don't want to depend on
# cmake 3.1, as it's not currently widely available. We only support
# building on Linux under GCC right now, so we might as well just hard
# code the flags.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++1y")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/usr/include/libnl3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-sign-compare -Wno-bool-compare")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable -Woverloaded-virtual -Wnon-virtual-dtor")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-maybe-uninitialized -Wdeprecated-declarations")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=deprecated-declarations")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DINCLUDE_L3 -DLONGS_ARE_64BITS")

# Tell CMake to also look in the directories where getdeps.sh installs
# our third-party dependencies.
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external.install")

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_BUILD_DIR})

find_package(folly CONFIG REQUIRED)
find_package(wangle CONFIG REQUIRED)
find_package(FBThrift CONFIG REQUIRED)

# Generate thrift deps and libraries
include(${CMAKE_SOURCE_DIR}/ThriftCppLibrary.cmake)

# Thrift libraries
add_thrift_cpp2_library(
  fb303_cpp2
  common/fb303/if/fb303.thrift
  SERVICES
    FacebookService
)
add_thrift_cpp2_library(
  network_address_cpp2
  common/network/if/Address.thrift
)
add_thrift_cpp2_library(
  highres_cpp2
  fboss/agent/if/highres.thrift
  SERVICES
    FbossHighresClient
)
add_thrift_cpp2_library(
  switch_config_cpp2
  fboss/agent/switch_config.thrift
  OPTIONS
    json
)
add_thrift_cpp2_library(
  switch_state_cpp2
  fboss/agent/switch_state.thrift
  OPTIONS
    json
    optionals
  DEPENDS
    switch_config_cpp2
)
add_thrift_cpp2_library(
  transceiver_cpp2
  fboss/qsfp_service/if/transceiver.thrift
  DEPENDS
    switch_config_cpp2
)
add_thrift_cpp2_library(
  optic_cpp2
  fboss/agent/if/optic.thrift
  DEPENDS
    transceiver_cpp2
)
add_thrift_cpp2_library(
  fboss_cpp2
  fboss/agent/if/fboss.thrift
)
add_thrift_cpp2_library(
  sflow_cpp2
  fboss/agent/if/sflow.thrift
)
add_thrift_cpp2_library(
  ctrl_cpp2
  fboss/agent/if/ctrl.thrift
  SERVICES
    FbossCtrl
    NeighborListenerClient
  DEPENDS
    fboss_cpp2
    fb303_cpp2
    network_address_cpp2
    optic_cpp2
    highres_cpp2
    transceiver_cpp2
)
add_thrift_cpp2_library(
  sim_ctrl_cpp2
  fboss/agent/hw/sim/sim_ctrl.thrift
  SERVICES
    SimCtrl
  DEPENDS
    fboss_cpp2
    ctrl_cpp2
)
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
add_thrift_cpp2_library(
  pcap_pubsub_cpp2
  fboss/pcap_distribution_service/if/pcap_pubsub.thrift
  SERVICES
    PcapPushSubscriber
    PcapSubscriber
)
add_thrift_cpp2_library(
  qsfp_cpp2
  fboss/qsfp_service/if/qsfp.thrift
  SERVICES
    QsfpService
  DEPENDS
    fb303_cpp2
    ctrl_cpp2
    fboss_cpp2
    transceiver_cpp2
    switch_config_cpp2
)
add_thrift_cpp2_library(
  netlink_manager_service_cpp2
  fboss/netlink_manager/if/netlink_manager_service.thrift
  SERVICES
    NetlinkManagerService
  DEPENDS
    fb303_cpp2
    ctrl_cpp2
)

# System libraries
find_library(USB usb-1.0)
find_library(PCAP pcap)
find_library(NETLINK3 nl-3)
find_library(NETLINKROUTE3 nl-route-3)
find_library(CURL curl)

find_library(OPENNSL opennsl
             PATHS ${CMAKE_SOURCE_DIR}/external/OpenNSL/bin/wedge-trident)
find_library(IPROUTE2 netlink PATHS ${CMAKE_SOURCE_DIR}/external/iproute2/lib)

include_directories(${CMAKE_SOURCE_DIR}/external/OpenNSL/include)
include_directories(${CMAKE_SOURCE_DIR}/external/iproute2/include)

add_executable(wedge_agent
    fboss/agent/platforms/wedge/WedgePlatform.cpp
    fboss/agent/platforms/wedge/WedgeProductInfo.cpp
    fboss/agent/platforms/wedge/WedgePort.cpp
    fboss/agent/platforms/wedge/wedge_agent.cpp
    fboss/agent/platforms/wedge/oss/WedgePort.cpp
    fboss/agent/platforms/wedge/oss/WedgePlatform.cpp
)

add_executable(netlink_manager
    fboss/netlink_manager/NlResources.cpp
    fboss/netlink_manager/NetlinkManager.cpp
    fboss/netlink_manager/NetlinkPoller.cpp
    fboss/netlink_manager/main.cpp
    fboss/netlink_manager/utils/AddressUtils.cpp
    fboss/netlink_manager/NetlinkManagerHandler.cpp
)

add_executable(qsfp_service
    fboss/qsfp_service/oss/QsfpServer.cpp
    fboss/qsfp_service/Main.cpp
    fboss/qsfp_service/QsfpServiceHandler.cpp
    fboss/qsfp_service/sff/QsfpModule.cpp
    fboss/qsfp_service/sff/SffFieldInfo.cpp
    fboss/qsfp_service/sff/oss/QsfpModule.cpp
    fboss/qsfp_service/platforms/wedge/WedgeManager.cpp
    fboss/qsfp_service/platforms/wedge/WedgeQsfp.cpp
    fboss/qsfp_service/platforms/wedge/Wedge100Manager.cpp
    fboss/qsfp_service/platforms/wedge/GalaxyManager.cpp
    fboss/qsfp_service/platforms/wedge/Wedge40Manager.cpp
    fboss/qsfp_service/platforms/wedge/WedgeManagerInit.cpp
    fboss/qsfp_service/platforms/wedge/oss/WedgeManagerInit.cpp
)

add_library(fboss_agent STATIC
    common/stats/ServiceData.cpp

    fboss/agent/ApplyThriftConfig.cpp
    fboss/agent/ArpCache.cpp
    fboss/agent/ArpHandler.cpp
    fboss/agent/BmcRestClient.cpp
    fboss/agent/capture/PcapFile.cpp
    fboss/agent/capture/PcapPkt.cpp
    fboss/agent/capture/PcapQueue.cpp
    fboss/agent/capture/PcapWriter.cpp
    fboss/agent/capture/PktCapture.cpp
    fboss/agent/capture/PktCaptureManager.cpp
    fboss/agent/DHCPv4Handler.cpp
    fboss/agent/DHCPv6Handler.cpp
    fboss/agent/HighresCounterSubscriptionHandler.cpp
    fboss/agent/HighresCounterUtil.cpp
    fboss/agent/hw/BufferStatsLogger.cpp
    fboss/agent/hw/bcm/BcmAclRange.cpp
    fboss/agent/hw/bcm/BcmAclTable.cpp
    fboss/agent/hw/bcm/BcmAPI.cpp
    fboss/agent/hw/bcm/BcmControlPlaneQueueManager.cpp
    fboss/agent/hw/bcm/BcmCosQueueManager.cpp
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
    fboss/agent/hw/bcm/BcmRtag7LoadBalancer.cpp
    fboss/agent/hw/bcm/BcmRtag7Module.cpp
    fboss/agent/hw/bcm/BcmRxPacket.cpp
    fboss/agent/hw/bcm/BcmSflowExporter.cpp
    fboss/agent/hw/bcm/BcmStats.cpp
    fboss/agent/hw/bcm/BcmStatUpdater.cpp
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
    fboss/agent/hw/bcm/oss/BcmControlPlaneQueueManager.cpp
    fboss/agent/hw/bcm/oss/BcmCosQueueManager.cpp
    fboss/agent/hw/bcm/oss/BcmEgress.cpp
    fboss/agent/hw/bcm/oss/BcmHost.cpp
    fboss/agent/hw/bcm/oss/BcmPort.cpp
    fboss/agent/hw/bcm/oss/BcmPortGroup.cpp
    fboss/agent/hw/bcm/oss/BcmPortQueueManager.cpp
    fboss/agent/hw/bcm/oss/BcmPortTable.cpp
    fboss/agent/hw/bcm/oss/BcmRtag7LoadBalancer.cpp
    fboss/agent/hw/bcm/oss/BcmRtag7Module.cpp
    fboss/agent/hw/bcm/oss/BcmStatUpdater.cpp
    fboss/agent/hw/bcm/oss/BcmSwitch.cpp
    fboss/agent/hw/bcm/oss/BcmSwitchEventCallback.cpp
    fboss/agent/hw/bcm/oss/BcmTrunk.cpp
    fboss/agent/hw/bcm/oss/BcmUnit.cpp
    fboss/agent/hw/bcm/oss/BcmWarmBootCache.cpp
    fboss/agent/hw/bcm/oss/BcmWarmBootHelper.cpp
    fboss/agent/hw/bcm/oss/BcmTableStats.cpp
    fboss/agent/hw/bcm/BcmAddressFBConvertors.cpp
    fboss/agent/hw/mock/MockHwSwitch.cpp
    fboss/agent/hw/mock/MockPlatform.cpp
    fboss/agent/hw/mock/MockRxPacket.cpp
    fboss/agent/hw/mock/MockTxPacket.cpp
    fboss/agent/hw/mock/MockTestHandle.cpp
    fboss/agent/hw/mock/MockableHwSwitch.cpp
    fboss/agent/hw/mock/MockablePlatform.cpp
    fboss/agent/hw/sim/SimHandler.cpp
    fboss/agent/hw/sim/SimSwitch.cpp
    fboss/agent/lldp/LinkNeighbor.cpp
    fboss/agent/lldp/LinkNeighborDB.cpp
    fboss/agent/ndp/IPv6RouteAdvertiser.cpp
    fboss/agent/HwSwitch.cpp
    fboss/agent/I2c.cpp
    fboss/agent/IPHeaderV4.cpp
    fboss/agent/IPv4Handler.cpp
    fboss/agent/IPv6Handler.cpp
    fboss/agent/lldp/LinkNeighbor.cpp
    fboss/agent/lldp/LinkNeighborDB.cpp
    fboss/agent/LacpController.cpp
    fboss/agent/LacpMachines.cpp
    fboss/agent/LacpTypes.cpp
    fboss/agent/LinkAggregationManager.cpp
    fboss/agent/LldpManager.cpp
    fboss/agent/LoadBalancerConfigApplier.cpp
    fboss/agent/Main.cpp
    fboss/agent/ndp/IPv6RouteAdvertiser.cpp
    fboss/agent/NdpCache.cpp
    fboss/agent/NeighborListenerClient.cpp
    fboss/agent/NeighborUpdater.cpp
    fboss/agent/NexthopToRouteCount.cpp
    fboss/agent/oss/ApplyThriftConfig.cpp
    fboss/agent/oss/Main.cpp
    fboss/agent/oss/RouteUpdateLogger.cpp
    fboss/agent/oss/SwSwitch.cpp
    fboss/agent/packet/ArpHdr.cpp
    fboss/agent/packet/DHCPv4Packet.cpp
    fboss/agent/packet/DHCPv6Packet.cpp
    fboss/agent/packet/EthHdr.cpp
    fboss/agent/packet/ICMPHdr.cpp
    fboss/agent/packet/IPv4Hdr.cpp
    fboss/agent/packet/IPv6Hdr.cpp
    fboss/agent/packet/LlcHdr.cpp
    fboss/agent/packet/NDPRouterAdvertisement.cpp
    fboss/agent/packet/PktUtil.cpp
    fboss/agent/Platform.cpp
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
    fboss/agent/PortRemediator.cpp
    fboss/agent/PortStats.cpp
    fboss/agent/PortUpdateHandler.cpp
    fboss/agent/RestClient.cpp
    fboss/agent/RouteUpdateLogger.cpp
    fboss/agent/RouteUpdateLoggingPrefixTracker.cpp
    fboss/agent/state/AclEntry.cpp
    fboss/agent/state/AclMap.cpp
    fboss/agent/state/AggregatePort.cpp
    fboss/agent/state/AggregatePortMap.cpp
    fboss/agent/state/ArpEntry.cpp
    fboss/agent/state/ArpResponseTable.cpp
    fboss/agent/state/ArpTable.cpp
    fboss/agent/state/ControlPlane.cpp
    fboss/agent/state/Interface.cpp
    fboss/agent/state/InterfaceMap.cpp
    fboss/agent/state/LoadBalancer.cpp
    fboss/agent/state/LoadBalancerMap.cpp
    fboss/agent/state/MatchAction.cpp
    fboss/agent/state/NdpEntry.cpp
    fboss/agent/state/NdpResponseTable.cpp
    fboss/agent/state/NdpTable.cpp
    fboss/agent/state/NeighborResponseTable.cpp
    fboss/agent/state/NodeBase.cpp
    fboss/agent/state/Port.cpp
    fboss/agent/state/PortMap.cpp
    fboss/agent/state/PortQueue.cpp
    fboss/agent/state/Route.cpp
    fboss/agent/state/RouteDelta.cpp
    fboss/agent/state/RouteNextHop.cpp
    fboss/agent/state/RouteNextHopEntry.cpp
    fboss/agent/state/RouteNextHopsMulti.cpp
    fboss/agent/state/RouteTable.cpp
    fboss/agent/state/RouteTableMap.cpp
    fboss/agent/state/RouteTableRib.cpp
    fboss/agent/state/RouteTypes.cpp
    fboss/agent/state/RouteUpdater.cpp
    fboss/agent/state/SflowCollector.cpp
    fboss/agent/state/SflowCollectorMap.cpp
    fboss/agent/state/StateDelta.cpp
    fboss/agent/state/StateUtils.cpp
    fboss/agent/state/SwitchState.cpp
    fboss/agent/state/Vlan.cpp
    fboss/agent/state/VlanMap.cpp
    fboss/agent/state/VlanMapDelta.cpp
    fboss/agent/types.cpp
    fboss/agent/SwitchStats.cpp
    fboss/agent/SwSwitch.cpp
    fboss/agent/ThriftHandler.cpp
    fboss/agent/ThreadHeartbeat.cpp
    fboss/agent/TunIntf.cpp
    fboss/agent/TunManager.cpp
    fboss/agent/UDPHeader.cpp
    fboss/agent/Utils.cpp

    fboss/lib/usb/GalaxyI2CBus.cpp
    fboss/lib/usb/BaseWedgeI2CBus.cpp
    fboss/lib/usb/BaseWedgeI2CBus.h
    fboss/lib/usb/CP2112.cpp
    fboss/lib/usb/CP2112.h
    fboss/lib/usb/PCA9548.cpp
    fboss/lib/usb/PCA9548MultiplexedBus.cpp
    fboss/lib/usb/PCA9548MuxedBus.cpp
    fboss/lib/usb/TransceiverI2CApi.h
    fboss/lib/usb/UsbDevice.cpp
    fboss/lib/usb/UsbDevice.h
    fboss/lib/usb/UsbError.h
    fboss/lib/usb/UsbHandle.cpp
    fboss/lib/usb/UsbHandle.h
    fboss/lib/usb/Wedge100I2CBus.cpp
    fboss/lib/usb/Wedge100I2CBus.h
    fboss/lib/usb/WedgeI2CBus.cpp
    fboss/lib/usb/WedgeI2CBus.h

    fboss/qsfp_service/oss/StatsPublisher.cpp
    fboss/qsfp_service/platforms/wedge/WedgeI2CBusLock.cpp
    fboss/qsfp_service/lib/QsfpClient.cpp
    fboss/qsfp_service/lib/QsfpCache.cpp

)
target_link_libraries(
  fboss_agent PUBLIC
    switch_state_cpp2
    sflow_cpp2
    ctrl_cpp2
    sim_ctrl_cpp2
    packettrace_cpp2
    bcmswitch_cpp2
    hardware_stats_cpp2
    pcap_pubsub_cpp2
    qsfp_cpp2
    netlink_manager_service_cpp2
    Folly::folly
    wangle::wangle
    FBThrift::thriftcpp2
    ${OPENNSL}
    ${PCAP}
    ${USB}
    ${IPROUTE2}
    ${NETLINK3}
    ${NETLINKROUTE3}
    ${CURL}
)

target_link_libraries(wedge_agent fboss_agent)
target_link_libraries(netlink_manager fboss_agent)
target_link_libraries(qsfp_service fboss_agent)


# TODO(rsher)
# depends on agent/hw/bcm/facebook/bcm_config.thrift
# investigate open sourcing
# add_executable(lldp_tool
#       fboss/util/lldp_tool.cpp
# )
#
# target_link_libraries(lldp_tool fboss-agent)

add_executable(cp2112_util
    fboss/util/cp2112_util.cpp
)
target_link_libraries(cp2112_util fboss_agent)

add_executable(wedge_qsfp_util
    fboss/util/wedge_qsfp_util.cpp
    fboss/util/oss/wedge_qsfp_util.cpp
)
target_link_libraries(wedge_qsfp_util fboss_agent)




# Unit Testing
add_definitions (-DIS_OSS=true)
find_package(Threads REQUIRED)
enable_testing()
set(GTEST_DIR "external/googletest")
include_directories(${GTEST_DIR}/googletest/include ${GTEST_DIR}/googlemock/include)
add_subdirectory(${GTEST_DIR} ${GTEST_DIR}.build)

# Don't include fboss/agent/test/ArpBenchmark.cpp
# It depends on the Sim implementation and needs its own target
add_executable(agent_test
       fboss/agent/test/TestUtils.cpp
       fboss/agent/test/ArpTest.cpp
       fboss/agent/test/CounterCache.cpp
       fboss/agent/test/DHCPv4HandlerTest.cpp
       fboss/agent/test/ICMPTest.cpp
       fboss/agent/test/IPv4Test.cpp
       fboss/agent/test/LldpManagerTest.cpp
       fboss/agent/test/MockTunManager.cpp
       fboss/agent/test/NDPTest.cpp
       fboss/agent/test/RouteUpdateLoggerTest.cpp
       fboss/agent/test/RouteUpdateLoggingTrackerTest.cpp
       fboss/agent/test/RoutingTest.cpp
       fboss/agent/test/StaticRoutes.cpp
       fboss/agent/test/ThriftTest.cpp
       fboss/agent/test/UDPTest.cpp
       fboss/agent/test/oss/Main.cpp
)
target_link_libraries(agent_test
    fboss_agent
    gtest
    gmock
    ${CMAKE_THREAD_LIBS_INIT}
)
add_test(test agent_test)

#TODO: Add tests from other folders aside from agent/test
