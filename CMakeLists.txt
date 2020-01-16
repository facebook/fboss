cmake_minimum_required(VERSION 3.8)

set(CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/build/fbcode_builder/CMake"
  ${CMAKE_MODULE_PATH})

set (CMAKE_MODULE_PATH
  "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if (POLICY CMP0054)
  # Enable CMP0054 to indicate that we want the new safer if() argument behavior
  cmake_policy(SET CMP0054 NEW)
endif()
project(FBOSS)

include(CMakeParseArguments)

set(CMAKE_CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-sign-compare -Wno-bool-compare")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable -Woverloaded-virtual -Wnon-virtual-dtor")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-maybe-uninitialized -Wdeprecated-declarations")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=deprecated-declarations")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DINCLUDE_L3 -DLONGS_ARE_64BITS")

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_BUILD_DIR})

find_package(GTest REQUIRED)
include_directories(
  ${GTEST_INCLUDE_DIRS}
)

find_package(GMock MODULE REQUIRED)
find_package(Gflags CONFIG REQUIRED)
find_package(Glog REQUIRED)
find_package(folly CONFIG REQUIRED)
find_package(fb303 CONFIG REQUIRED)
find_package(yarpl CONFIG REQUIRED)
find_package(rsocket CONFIG)
find_package(fizz CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(wangle CONFIG REQUIRED)
find_package(FBThrift CONFIG REQUIRED)
include_directories(${FB303_INCLUDE_DIR})

find_library(OPENNSA libopennsa.a opennsa)
find_path(OPENNSA_INCLUDE_DIR NAMES bcm/tx.h)
include_directories(${OPENNSA_INCLUDE_DIR})

message(STATUS "Found OPENNSA: ${OPENNSA}")
message(STATUS "Found OPENNSA_INCLUDE_DIR: ${OPENNSA_INCLUDE_DIR}")


find_path(SAI_INCLUDE_DIR NAMES sai.h)
include_directories(${SAI_INCLUDE_DIR})

message(STATUS "Found SAI_INCLUDE_DIR: ${SAI_INCLUDE_DIR}")

find_library(LIBNL nl-3)
find_path(LIBNL_INCLUDE_DIR NAMES libnl3/netlink/socket.h)
include_directories(${LIBNL_INCLUDE_DIR})
include_directories(${LIBNL_INCLUDE_DIR}/libnl3)

find_library(IPROUTE2 netlink)
find_path(IPROUTE2_INCLUDE_DIR NAMES libnetlink.h)
include_directories(${IPROUTE2_INCLUDE_DIR})

# Generate thrift deps and libraries
include(FBThriftCppLibrary)

# Thrift libraries
add_fbthrift_cpp_library(
  fb303_cpp2
  common/fb303/if/fb303.thrift
  SERVICES
    FacebookService
)
add_fbthrift_cpp_library(
  network_address_cpp2
  common/network/if/Address.thrift
)
add_fbthrift_cpp_library(
  mpls_cpp2
  fboss/agent/if/mpls.thrift
  OPTIONS
    json
)
add_fbthrift_cpp_library(
  switch_config_cpp2
  fboss/agent/switch_config.thrift
  OPTIONS
    json
  DEPENDS
    mpls_cpp2
)
add_fbthrift_cpp_library(
  agent_config_cpp2
  fboss/agent/agent_config.thrift
  OPTIONS
    json
  DEPENDS
    platform_config_cpp2
    switch_config_cpp2
)
add_fbthrift_cpp_library(
  platform_config_cpp2
  fboss/agent/platform_config.thrift
  OPTIONS
    json
  DEPENDS
    bcm_config_cpp2
    asic_config_cpp2
    phy_cpp2
)
add_fbthrift_cpp_library(
  switch_state_cpp2
  fboss/agent/switch_state.thrift
  OPTIONS
    json
    optionals
  DEPENDS
    switch_config_cpp2
)
add_fbthrift_cpp_library(
  phy_cpp2
  fboss/lib/phy/phy.thrift
  OPTIONS
    json
  DEPENDS
    switch_config_cpp2
)
add_fbthrift_cpp_library(
  transceiver_cpp2
  fboss/qsfp_service/if/transceiver.thrift
)
add_fbthrift_cpp_library(
  optic_cpp2
  fboss/agent/if/optic.thrift
  DEPENDS
    transceiver_cpp2
)
add_fbthrift_cpp_library(
  fboss_cpp2
  fboss/agent/if/fboss.thrift
)
add_fbthrift_cpp_library(
  sflow_cpp2
  fboss/agent/if/sflow.thrift
)
add_fbthrift_cpp_library(
  ctrl_cpp2
  fboss/agent/if/ctrl.thrift
  SERVICES
    FbossCtrl
    NeighborListenerClient
  DEPENDS
    fboss_cpp2
    fb303_cpp2
    mpls_cpp2
    network_address_cpp2
    optic_cpp2
    transceiver_cpp2
)
add_fbthrift_cpp_library(
  sim_ctrl_cpp2
  fboss/agent/hw/sim/sim_ctrl.thrift
  SERVICES
    SimCtrl
  DEPENDS
    fboss_cpp2
    ctrl_cpp2
)
add_fbthrift_cpp_library(
  bcm_config_cpp2
  fboss/agent/hw/bcm/bcm_config.thrift
  OPTIONS
    json
)
add_fbthrift_cpp_library(
  asic_config_cpp2
  fboss/agent/hw/sai/config/asic_config.thrift
  OPTIONS
    json
)
add_fbthrift_cpp_library(
  packettrace_cpp2
  fboss/agent/hw/bcm/packettrace.thrift
)
add_fbthrift_cpp_library(
  bcmswitch_cpp2
  fboss/agent/hw/bcm/bcmswitch.thrift
)
add_fbthrift_cpp_library(
  hardware_stats_cpp2
  fboss/agent/hw/hardware_stats.thrift
)
add_fbthrift_cpp_library(
  pcap_pubsub_cpp2
  fboss/pcap_distribution_service/if/pcap_pubsub.thrift
  SERVICES
    PcapPushSubscriber
    PcapSubscriber
)
add_fbthrift_cpp_library(
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
    mpls_cpp2
)
add_fbthrift_cpp_library(
  netlink_manager_service_cpp2
  fboss/netlink_manager/if/netlink_manager_service.thrift
  SERVICES
    NetlinkManagerService
  DEPENDS
    fb303_cpp2
    ctrl_cpp2
)

find_library(USB usb-1.0)
find_path(USB_INCLUDE_DIR NAMES libusb-1.0/libusb.h)
include_directories(${USB_INCLUDE_DIR})

find_library(NETLINK3 nl-3)
find_library(NETLINKROUTE3 libnl-route-3.a nl-route-3)

find_library(CURL curl)
find_path(CURL_INCLUDE_DIR NAMES curl/curl.h)
include_directories(${CURL_INCLUDE_DIR})

find_library(MNL mnl)
find_library(SODIUM sodium)

add_executable(wedge_agent
    fboss/agent/platforms/wedge/WedgePlatform.cpp
    fboss/agent/platforms/common/PlatformProductInfo.cpp
    fboss/agent/platforms/wedge/WedgePort.cpp
    fboss/agent/platforms/wedge/wedge_agent.cpp
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
    fboss/qsfp_service/module/QsfpModule.cpp
    fboss/qsfp_service/module/oss/QsfpModule.cpp
    fboss/qsfp_service/module/sff/SffFieldInfo.cpp
    fboss/qsfp_service/module/sff/SffModule.cpp
    fboss/qsfp_service/module/oss/SffModule.cpp
    fboss/qsfp_service/platforms/wedge/WedgeManager.cpp
    fboss/qsfp_service/platforms/wedge/WedgeQsfp.cpp
    fboss/qsfp_service/platforms/wedge/Wedge100Manager.cpp
    fboss/qsfp_service/platforms/wedge/GalaxyManager.cpp
    fboss/qsfp_service/platforms/wedge/Wedge40Manager.cpp
    fboss/qsfp_service/platforms/wedge/WedgeManagerInit.cpp
    fboss/qsfp_service/platforms/wedge/oss/WedgeManagerInit.cpp
)

add_library(bcm_switch_ensemble STATIC
    fboss/agent/hw/test/HwSwitchEnsemble.cpp
    fboss/agent/hw/test/HwLinkStateToggler.cpp
    fboss/agent/hw/bcm/tests/BcmSwitchEnsemble.cpp
    fboss/agent/hw/bcm/tests/HwSwitchEnsembleFactory.cpp
)

add_executable(bcm_test
    fboss/agent/hw/test/HwTest.cpp
    fboss/agent/hw/test/HwTestStatUtils.cpp
    fboss/agent/hw/test/HwVlanTests.cpp
    fboss/agent/hw/test/HwRouteScaleTest.cpp
    fboss/agent/hw/test/HwRouteOverflowTest.cpp
    fboss/agent/hw/test/ConfigFactory.cpp
    fboss/agent/hw/test/HwLinkStateDependentTest.cpp
    fboss/agent/hw/test/HwTestConstants.cpp
    fboss/agent/hw/test/HwTestCoppUtils.cpp
    fboss/agent/hw/test/Main.cpp
    fboss/agent/hw/bcm/tests/BcmTest.cpp
    fboss/agent/hw/bcm/tests/BcmPortUtils.cpp
    fboss/agent/hw/bcm/tests/HwVlanUtils.cpp
    fboss/agent/hw/bcm/tests/HwTestMacUtils.cpp
    fboss/agent/hw/bcm/tests/BcmColdBootStateTests.cpp
    fboss/agent/hw/bcm/tests/BcmQueueStatCollectionTests.cpp
    fboss/agent/hw/bcm/tests/oss/BcmSwitchEnsemble.cpp
    fboss/agent/hw/bcm/tests/oss/BcmPortUtils.cpp
    fboss/agent/hw/bcm/tests/oss/HwTestCoppUtils.cpp
    fboss/agent/platforms/test_platforms/BcmTestPort.cpp
    fboss/agent/platforms/test_platforms/BcmTestPlatform.cpp
    fboss/agent/platforms/test_platforms/BcmTestWedgePlatform.cpp
    fboss/agent/platforms/test_platforms/BcmTestWedge40Platform.cpp
    fboss/agent/platforms/test_platforms/BcmTestWedge40Port.cpp
    fboss/agent/platforms/test_platforms/BcmTestWedge100Platform.cpp
    fboss/agent/platforms/test_platforms/BcmTestWedge100Port.cpp
    fboss/agent/platforms/test_platforms/BcmTestGalaxyPlatform.cpp
    fboss/agent/platforms/test_platforms/BcmTestGalaxyPort.cpp
    fboss/agent/platforms/test_platforms/BcmTestMinipackPlatform.cpp
    fboss/agent/platforms/test_platforms/BcmTestMinipackPort.cpp
    fboss/agent/platforms/test_platforms/BcmTestYampPlatform.cpp
    fboss/agent/platforms/test_platforms/BcmTestYampPort.cpp
    fboss/agent/platforms/test_platforms/BcmTestWedge400Platform.cpp
    fboss/agent/platforms/test_platforms/BcmTestWedge400Port.cpp
    fboss/agent/platforms/test_platforms/FakeBcmTestPlatform.cpp
    fboss/agent/platforms/test_platforms/FakeBcmTestPort.cpp
    fboss/agent/platforms/test_platforms/BcmTestPlatform.cpp
    fboss/agent/platforms/test_platforms/BcmTestPort.cpp
    fboss/agent/platforms/test_platforms/CreateTestPlatform.cpp
    fboss/agent/platforms/test_platforms/oss/BcmTestWedgeTomahawkPlatform.cpp
    fboss/agent/platforms/test_platforms/oss/BcmTestWedgeTomahawk3Platform.cpp
    fboss/agent/platforms/test_platforms/oss/BcmTestWedge40Platform.cpp
    fboss/agent/platforms/test_platforms/oss/FakeBcmTestPlatform.cpp
)

add_library(switch_asics STATIC
    fboss/agent/hw/switch_asics/FakeAsic.h
    fboss/agent/hw/switch_asics/HwAsic.h
    fboss/agent/hw/switch_asics/Tomahawk3Asic.h
    fboss/agent/hw/switch_asics/TomahawkAsic.h
    fboss/agent/hw/switch_asics/Trident2Asic.h
    fboss/agent/hw/switch_asics/oss/Tomahawk3Asic.cpp
    fboss/agent/hw/switch_asics/oss/TomahawkAsic.cpp
    fboss/agent/hw/switch_asics/oss/Trident2Asic.cpp
)

add_library(fboss_agent STATIC
    fboss/agent/AgentConfig.cpp
    fboss/agent/AggregatePortStats.cpp
    fboss/agent/AlpmUtils.cpp
    fboss/agent/ApplyThriftConfig.cpp
    fboss/agent/ArpCache.cpp
    fboss/agent/ArpHandler.cpp
    fboss/agent/StandaloneRibConversions.cpp
    fboss/agent/capture/PcapFile.cpp
    fboss/agent/capture/PcapPkt.cpp
    fboss/agent/capture/PcapQueue.cpp
    fboss/agent/capture/PcapWriter.cpp
    fboss/agent/capture/PktCapture.cpp
    fboss/agent/capture/PktCaptureManager.cpp
    fboss/agent/DHCPv4Handler.cpp
    fboss/agent/DHCPv6Handler.cpp
    fboss/agent/L2Entry.cpp
    fboss/agent/hw/BufferStatsLogger.cpp
    fboss/agent/hw/bcm/BcmAclTable.cpp
    fboss/agent/hw/bcm/BcmAPI.cpp
    fboss/agent/hw/bcm/BcmConfig.cpp
    fboss/agent/hw/bcm/BcmControlPlane.cpp
    fboss/agent/hw/bcm/BcmControlPlaneQueueManager.cpp
    fboss/agent/hw/bcm/BcmCosQueueManager.cpp
    fboss/agent/hw/bcm/BcmEgress.cpp
    fboss/agent/hw/bcm/BcmHost.cpp
    fboss/agent/hw/bcm/BcmHostKey.cpp
    fboss/agent/hw/bcm/BcmIntf.cpp
    fboss/agent/hw/bcm/BcmMirrorTable.cpp
    fboss/agent/hw/bcm/BcmLabeledTunnel.cpp
    fboss/agent/hw/bcm/BcmPortDescriptor.cpp
    fboss/agent/hw/bcm/BcmPlatform.cpp
    fboss/agent/hw/bcm/BcmPlatformPort.cpp
    fboss/agent/hw/bcm/BcmPort.cpp
    fboss/agent/hw/bcm/BcmPortUtils.cpp
    fboss/agent/hw/bcm/BcmPortGroup.cpp
    fboss/agent/hw/bcm/BcmPortQueueManager.cpp
    fboss/agent/hw/bcm/BcmPortTable.cpp
    fboss/agent/hw/bcm/BcmQosMap.cpp
    fboss/agent/hw/bcm/BcmQosPolicy.cpp
    fboss/agent/hw/bcm/BcmQosPolicyTable.cpp
    fboss/agent/hw/bcm/BcmRoute.cpp
    fboss/agent/hw/bcm/BcmRtag7LoadBalancer.cpp
    fboss/agent/hw/bcm/BcmRtag7Module.cpp
    fboss/agent/hw/bcm/BcmRxPacket.cpp
    fboss/agent/hw/bcm/BcmSflowExporter.cpp
    fboss/agent/hw/bcm/BcmStats.cpp
    fboss/agent/hw/bcm/BcmStatUpdater.cpp
    fboss/agent/hw/bcm/BcmSwitch.cpp
    fboss/agent/hw/bcm/BcmSwitchEventUtils.cpp
    fboss/agent/hw/bcm/BcmTrunk.cpp
    fboss/agent/hw/bcm/BcmTrunkStats.cpp
    fboss/agent/hw/bcm/BcmTrunkTable.cpp
    fboss/agent/hw/bcm/BcmTxPacket.cpp
    fboss/agent/hw/bcm/BcmUnit.cpp
    fboss/agent/hw/bcm/BcmWarmBootCache.cpp
    fboss/agent/hw/bcm/BcmWarmBootHelper.cpp
    fboss/agent/hw/bcm/BcmWarmBootState.cpp
    fboss/agent/hw/bcm/CounterUtils.cpp
    fboss/agent/hw/bcm/PortAndEgressIdsMap.cpp
    fboss/agent/hw/bcm/BcmEgressManager.cpp
    fboss/agent/hw/bcm/BcmNextHop.cpp
    fboss/agent/hw/bcm/BcmMultiPathNextHop.cpp
    fboss/agent/hw/bcm/BcmLabeledEgress.cpp
    fboss/agent/hw/bcm/BcmLabeledTunnelEgress.cpp
    fboss/agent/hw/bcm/BcmClassIDUtil.cpp
    fboss/agent/hw/bcm/BcmSwitchSettings.cpp
    fboss/agent/hw/bcm/BcmMacTable.cpp
    fboss/agent/hw/bcm/oss/BcmAclEntry.cpp
    fboss/agent/hw/bcm/oss/BcmAclStat.cpp
    fboss/agent/hw/bcm/oss/BcmAPI.cpp
    fboss/agent/hw/bcm/oss/BcmControlPlane.cpp
    fboss/agent/hw/bcm/oss/BcmControlPlaneQueueManager.cpp
    fboss/agent/hw/bcm/oss/BcmCosQueueManager.cpp
    fboss/agent/hw/bcm/oss/BcmHost.cpp
    fboss/agent/hw/bcm/oss/BcmIntf.cpp
    fboss/agent/hw/bcm/oss/BcmMirror.cpp
    fboss/agent/hw/bcm/oss/BcmLabelMap.cpp
    fboss/agent/hw/bcm/oss/BcmLabeledTunnel.cpp
    fboss/agent/hw/bcm/oss/BcmPort.cpp
    fboss/agent/hw/bcm/oss/BcmPortGroup.cpp
    fboss/agent/hw/bcm/oss/BcmPortQueueManager.cpp
    fboss/agent/hw/bcm/oss/BcmPortTable.cpp
    fboss/agent/hw/bcm/oss/BcmQosMap.cpp
    fboss/agent/hw/bcm/oss/BcmRtag7Module.cpp
    fboss/agent/hw/bcm/oss/BcmStatUpdater.cpp
    fboss/agent/hw/bcm/oss/BcmSwitch.cpp
    fboss/agent/hw/bcm/oss/BcmSwitchEventCallback.cpp
    fboss/agent/hw/bcm/oss/BcmUnit.cpp
    fboss/agent/hw/bcm/oss/BcmWarmBootCache.cpp
    fboss/agent/hw/bcm/oss/BcmWarmBootHelper.cpp
    fboss/agent/hw/bcm/oss/BcmTableStats.cpp
    fboss/agent/hw/bcm/oss/BcmBstStatsMgr.cpp
    fboss/agent/hw/bcm/oss/BcmLabeledEgress.cpp
    fboss/agent/hw/bcm/oss/BcmLabeledTunnelEgress.cpp
    fboss/agent/hw/bcm/oss/BcmTypesImpl.cpp
    fboss/agent/hw/bcm/BcmAddressFBConvertors.cpp
    fboss/agent/hw/mock/MockHwSwitch.cpp
    fboss/agent/hw/mock/MockPlatform.cpp
    fboss/agent/hw/mock/MockRxPacket.cpp
    fboss/agent/hw/mock/MockTxPacket.cpp
    fboss/agent/hw/mock/MockTestHandle.cpp
    fboss/agent/hw/sim/SimHandler.cpp
    fboss/agent/hw/sim/SimSwitch.cpp
    fboss/agent/lldp/LinkNeighbor.cpp
    fboss/agent/lldp/LinkNeighborDB.cpp
    fboss/agent/ndp/IPv6RouteAdvertiser.cpp
    fboss/agent/HwSwitch.cpp
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
    fboss/agent/SetupThrift.cpp
    fboss/agent/MirrorManager.cpp
    fboss/agent/MirrorManagerImpl.cpp
    fboss/agent/LookupClassUpdater.cpp
    fboss/agent/MacTableManager.cpp
    fboss/agent/MacTableUtils.cpp
    fboss/agent/ResolvedNexthopMonitor.cpp
    fboss/agent/ResolvedNexthopProbe.cpp
    fboss/agent/ResolvedNexthopProbeScheduler.cpp
    fboss/agent/ndp/IPv6RouteAdvertiser.cpp
    fboss/agent/NdpCache.cpp
    fboss/agent/NeighborListenerClient.cpp
    fboss/agent/NeighborUpdater.cpp
    fboss/agent/NeighborUpdaterImpl.cpp
    fboss/agent/oss/AggregatePortStats.cpp
    fboss/agent/oss/FbossInit.cpp
    fboss/agent/oss/Main.cpp
    fboss/agent/oss/SetupThrift.cpp
    fboss/agent/oss/RouteUpdateLogger.cpp
    fboss/agent/oss/SwSwitch.cpp
    fboss/agent/oss/Utils.cpp
    fboss/agent/packet/ArpHdr.cpp
    fboss/agent/packet/DHCPv4Packet.cpp
    fboss/agent/packet/DHCPv6Packet.cpp
    fboss/agent/packet/EthHdr.cpp
    fboss/agent/packet/ICMPHdr.cpp
    fboss/agent/packet/IPv4Hdr.cpp
    fboss/agent/packet/IPv6Hdr.cpp
    fboss/agent/packet/LlcHdr.cpp
    fboss/agent/packet/NDP.cpp
    fboss/agent/packet/NDPRouterAdvertisement.cpp
    fboss/agent/packet/PktUtil.cpp
    fboss/agent/packet/SflowStructs.cpp
    fboss/agent/packet/TCPHeader.cpp
    fboss/agent/packet/UDPHeader.cpp
    fboss/agent/Platform.cpp
    fboss/agent/PlatformPort.cpp
    fboss/agent/platforms/wedge/oss/GalaxyPlatform.cpp
    fboss/agent/platforms/wedge/oss/GalaxyPort.cpp
    fboss/agent/platforms/wedge/oss/WedgePlatform.cpp
    fboss/agent/platforms/wedge/oss/Wedge40Platform.cpp
    fboss/agent/platforms/wedge/oss/Wedge100Platform.cpp
    fboss/agent/platforms/wedge/oss/Wedge40Port.cpp
    fboss/agent/platforms/wedge/oss/Wedge100Port.cpp
    fboss/agent/platforms/wedge/oss/WedgePlatformInit.cpp
    fboss/agent/platforms/common/oss/PlatformProductInfo.cpp
    fboss/agent/platforms/wedge/oss/WedgeTomahawkPlatform.cpp
    fboss/agent/platforms/wedge/wedge_agent.cpp
    fboss/agent/platforms/wedge/WedgePlatform.cpp
    fboss/agent/platforms/wedge/Wedge40Platform.cpp
    fboss/agent/platforms/wedge/Wedge40Port.cpp
    fboss/agent/platforms/wedge/FakeWedge40Platform.cpp
    fboss/agent/platforms/wedge/Wedge100Platform.cpp
    fboss/agent/platforms/wedge/GalaxyPort.cpp
    fboss/agent/platforms/wedge/GalaxyLCPlatform.cpp
    fboss/agent/platforms/wedge/GalaxyFCPlatform.cpp
    fboss/agent/platforms/wedge/WedgePort.cpp
    fboss/agent/platforms/wedge/Wedge100Port.cpp
    fboss/agent/platforms/common/PlatformProductInfo.cpp
    fboss/agent/platforms/wedge/WedgePlatformInit.cpp
    fboss/agent/PortStats.cpp
    fboss/agent/PortUpdateHandler.cpp
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
    fboss/agent/state/ForwardingInformationBase.cpp
    fboss/agent/state/ForwardingInformationBaseContainer.cpp
    fboss/agent/state/ForwardingInformationBaseDelta.cpp
    fboss/agent/state/ForwardingInformationBaseMap.cpp
    fboss/agent/state/Interface.cpp
    fboss/agent/state/InterfaceMap.cpp
    fboss/agent/state/LabelForwardingAction.cpp
    fboss/agent/state/LabelForwardingEntry.cpp
    fboss/agent/state/LabelForwardingInformationBase.cpp
    fboss/agent/state/LoadBalancer.cpp
    fboss/agent/state/LoadBalancerMap.cpp
    fboss/agent/state/MacEntry.cpp
    fboss/agent/state/MacTable.cpp
    fboss/agent/state/Mirror.cpp
    fboss/agent/state/MirrorMap.cpp
    fboss/agent/state/MatchAction.cpp
    fboss/agent/state/NdpEntry.cpp
    fboss/agent/state/NdpResponseTable.cpp
    fboss/agent/state/NdpTable.cpp
    fboss/agent/state/NeighborResponseTable.cpp
    fboss/agent/state/NodeBase.cpp
    fboss/agent/state/Port.cpp
    fboss/agent/state/PortMap.cpp
    fboss/agent/state/PortQueue.cpp
    fboss/agent/state/QosPolicy.cpp
    fboss/agent/state/QosPolicyMap.cpp
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
    fboss/agent/state/SwitchSettings.cpp
    fboss/agent/types.cpp
    fboss/agent/RestartTimeTracker.cpp
    fboss/agent/SwitchStats.cpp
    fboss/agent/SwSwitch.cpp
    fboss/agent/ThriftHandler.cpp
    fboss/agent/ThreadHeartbeat.cpp
    fboss/agent/TunIntf.cpp
    fboss/agent/TunManager.cpp
    fboss/agent/Utils.cpp
    fboss/agent/rib/ConfigApplier.cpp
    fboss/agent/rib/ForwardingInformationBaseUpdater.cpp
    fboss/agent/rib/Route.cpp
    fboss/agent/rib/RouteNextHop.cpp
    fboss/agent/rib/RouteNextHopEntry.cpp
    fboss/agent/rib/RouteNextHopsMulti.cpp
    fboss/agent/rib/RouteTypes.cpp
    fboss/agent/rib/RouteUpdater.cpp
    fboss/agent/rib/RoutingInformationBase.cpp

    fboss/agent/test/EcmpSetupHelper.cpp
    fboss/agent/test/ResourceLibUtil.cpp
    fboss/agent/test/RouteDistributionGenerator.cpp
    fboss/agent/test/RouteScaleGenerators.cpp

    fboss/lib/config/PlatformConfigUtils.cpp
    fboss/lib/config/PlatformConfigUtils.h
    fboss/lib/usb/GalaxyI2CBus.cpp
    fboss/lib/usb/BaseWedgeI2CBus.cpp
    fboss/lib/usb/BaseWedgeI2CBus.h
    fboss/lib/RestClient.cpp
    fboss/lib/BmcRestClient.cpp
    fboss/lib/usb/CP2112.cpp
    fboss/lib/usb/CP2112.h
    fboss/lib/usb/PCA9548.cpp
    fboss/lib/usb/PCA9548MultiplexedBus.cpp
    fboss/lib/usb/PCA9548MuxedBus.cpp
    fboss/lib/i2c/PCA9541.cpp
    fboss/lib/i2c/PCA9541.h
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

    fboss/lib/ExponentialBackoff.cpp
    fboss/lib/ExponentialBackoff.h
    fboss/lib/LogThriftCall.cpp

    fboss/qsfp_service/oss/StatsPublisher.cpp
    fboss/qsfp_service/platforms/wedge/WedgeI2CBusLock.cpp
    fboss/qsfp_service/lib/QsfpClient.cpp
    fboss/qsfp_service/lib/QsfpCache.cpp

)

target_link_libraries(
  fboss_agent PUBLIC
    agent_config_cpp2
    switch_asics
    switch_config_cpp2
    switch_state_cpp2
    sflow_cpp2
    ctrl_cpp2
    sim_ctrl_cpp2
    packettrace_cpp2
    bcmswitch_cpp2
    hardware_stats_cpp2
    mpls_cpp2
    pcap_pubsub_cpp2
    qsfp_cpp2
    netlink_manager_service_cpp2
    Folly::folly
    wangle::wangle
    fb303::fb303
    FBThrift::thriftcpp2
    ${USB}
    ${IPROUTE2}
    ${NETLINK3}
    ${NETLINKROUTE3}
    ${CURL}
    ${SODIUM}
    ${MNL}
    ${OPENNSA}
)

target_link_libraries(switch_asics fboss_agent)
target_link_libraries(bcm_switch_ensemble fboss_agent)
target_link_libraries(wedge_agent fboss_agent)
target_link_libraries(netlink_manager fboss_agent)
target_link_libraries(qsfp_service fboss_agent)

target_compile_definitions(bcm_test
  PUBLIC
    ${LIBGMOCK_DEFINES}
)

target_include_directories(bcm_test
  PUBLIC
    ${LIBGMOCK_INCLUDE_DIR}
)

target_link_libraries(bcm_test
  PUBLIC
    fboss_agent
    bcm_switch_ensemble
    ${GTEST}
    ${LIBGMOCK_LIBRARIES}
)

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

# Don't include fboss/agent/test/ArpBenchmark.cpp
# It depends on the Sim implementation and needs its own target
add_executable(agent_test
       fboss/agent/test/TestUtils.cpp
       fboss/agent/test/ArpTest.cpp
       fboss/agent/test/CounterCache.cpp
       fboss/agent/test/DHCPv4HandlerTest.cpp
       fboss/agent/test/EcmpSetupHelper.cpp
       fboss/agent/test/ICMPTest.cpp
       fboss/agent/test/IPv4Test.cpp
       fboss/agent/test/LldpManagerTest.cpp
       fboss/agent/test/MacTableManagerTests.cpp
       fboss/agent/test/MacTableUtilsTests.cpp
       fboss/agent/test/MockTunManager.cpp
       fboss/agent/test/NDPTest.cpp
       fboss/agent/test/ResourceLibUtil.cpp
       fboss/agent/test/ResourceLibUtilTest.cpp
       fboss/agent/test/RouteGeneratorTestUtils.cpp
       fboss/agent/test/RouteDistributionGenerator.cpp
       fboss/agent/test/RouteDistributionGeneratorTest.cpp
       fboss/agent/test/RouteUpdateLoggerTest.cpp
       fboss/agent/test/RouteUpdateLoggingTrackerTest.cpp
       fboss/agent/test/ResourceLibUtilTest.cpp
       fboss/agent/test/RouteDistributionGeneratorTest.cpp
       fboss/agent/test/RouteScaleGeneratorsTest.cpp
       fboss/agent/test/StaticRoutes.cpp
       fboss/agent/test/TestPacketFactory.cpp
       fboss/agent/test/ThriftTest.cpp
       fboss/agent/test/TrunkUtils.cpp
       fboss/agent/test/TunInterfaceTest.cpp
       fboss/agent/test/UDPTest.cpp
       fboss/agent/test/RouteDistributionGenerator.cpp
       fboss/agent/test/RouteScaleGenerators.cpp
       fboss/agent/test/RouteDistributionGeneratorTest.cpp
       fboss/agent/test/RouteScaleGeneratorsTest.cpp
       fboss/agent/test/oss/Main.cpp
)

target_compile_definitions(agent_test
  PUBLIC
    ${LIBGMOCK_DEFINES}
)

target_include_directories(agent_test
  PUBLIC
    ${LIBGMOCK_INCLUDE_DIR}
)

target_link_libraries(agent_test
    fboss_agent
    ${GTEST}
    ${CMAKE_THREAD_LIBS_INIT}
    ${LIBGMOCK_LIBRARIES}
)
add_test(test agent_test)

#TODO: Add tests from other folders aside from agent/test

install(TARGETS bcm_test)
install(TARGETS wedge_agent)

# Build and install SAI executables
include(SaiExecutables)
