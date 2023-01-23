cmake_minimum_required(VERSION 3.8)

option(WITH_ASAN "Turn ON address sanitizer build options" OFF)
if ($ENV{WITH_ASAN})
  message(STATUS "ENV WITH_ASAN is set")
  set (WITH_ASAN ON)
endif()

if (WITH_ASAN)
  message(STATUS "Building WITH_ASAN")
endif()

option(SAI_ONLY "Build SAI binaries only" OFF)
if ($ENV{SAI_ONLY})
  message(STATUS "ENV SAI_ONLY is set")
  set (SAI_ONLY ON)
endif()

if (SAI_ONLY)
  message(STATUS "Building SAI binaries only")
endif()

option(BENCHMARK_INSTALL "Installation of benchmark test binaries" OFF)
if ($ENV{BENCHMARK_INSTALL})
  message(STATUS "ENV BENCHMARK_INSTALL is set")
  set (BENCHMARK_INSTALL ON)
endif()

if (BENCHMARK_INSTALL)
  message(STATUS "Configured to install benchmark binaries")
endif()

option(BUILD_SAI_FAKE_BENCHMARKS "Build benchmarks binaries for fake_sai" OFF)
if ($ENV{BUILD_SAI_FAKE_BENCHMARKS})
  message(STATUS "ENV BUILD_SAI_FAKE_BENCHMARKS is set")
  set (BUILD_SAI_FAKE_BENCHMARKS ON)
endif()

if (BUILD_SAI_FAKE_BENCHMARKS)
  message(STATUS "Building benchmarks binaries for fake_sai")
endif()

option(SKIP_ALL_INSTALL "Installation of fboss binaries" OFF)
if ($ENV{SKIP_ALL_INSTALL})
  message(STATUS "ENV SKIP_ALL_INSTALL is set")
  set (SKIP_ALL_INSTALL ON)
endif()

if (SKIP_ALL_INSTALL)
  install(FILES README.md DESTINATION .)
  macro (install)
    message(STATUS "Skipping installing ${ARGV} fboss artifact")
    _install(${ARGV} EXCLUDE_FROM_ALL)
  endmacro ()
endif()

option(SAI_TAJO_IMPL "Build SAI api with tajo extensions" OFF)
if ($ENV{SAI_TAJO_IMPL})
  message(STATUS "ENV SAI_TAJO_IMPL is set")
  set (SAI_TAJO_IMPL ON)
endif()

option(SAI_BRCM_IMPL "Build SAI api with BRCM-SAI extensions" OFF)
if ($ENV{SAI_BRCM_IMPL})
  message(STATUS "ENV SAI_BRCM_IMPL is set")
  set (SAI_BRCM_IMPL ON)
endif()

if (SAI_BRCM_IMPL)
  # TODO (skhare) IS_OSS_BRCM_SAI is only a short-term change to unblock.
  # Refactor/fix the code and then remove this flag.
  add_definitions (-DIS_OSS_BRCM_SAI=true)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSAI_VERSION_7_2_0_0_ODP")
  set(SAI_VER_MAJOR "1")
  set(SAI_VER_MINOR "10")
  set(SAI_VER_RELEASE "2")
else()
  set(SAI_VER_MAJOR "1")
  set(SAI_VER_MINOR "7")
  set(SAI_VER_RELEASE "1")
endif()

message(STATUS "Building for SAI Spec ${SAI_VER_MAJOR}.${SAI_VER_MINOR}.${SAI_VER_RELEASE}")
set(SAI_VER_SUFFIX "${SAI_VER_MAJOR}.${SAI_VER_MINOR}.${SAI_VER_RELEASE}")

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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-maybe-uninitialized")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
if (WITH_ASAN)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -static-libasan")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-sanitize-recover=all")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-common")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize-address-use-after-scope")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-sanitize=alignment")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-sanitize=null")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-sanitize=vptr")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libasan")
endif()

# TODO (skhare) Once CMakeLists.txt is modified to follow buck hierarchy in
# cmake/*, set BCM-specific flags for BCM libs only
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DINCLUDE_L3 -DLONGS_ARE_64BITS \
    -DBCM_ESW_SUPPORT -DBCMDRD_INCLUDE_CUSTOM_CONFIG \
    -DBCMDRD_CONFIG_INCLUDE_BCM56990_Bx=1 \
    -DBCMDRD_CONFIG_INCLUDE_CHIP_DEFAULT=0 \
    -DINCLUDE_PSTATS -DBCM_RPC_SUPPORT -DBC")

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_BUILD_DIR})

find_package(GTest REQUIRED)
include_directories(
  ${GTEST_INCLUDE_DIRS}
)

find_package(GMock MODULE REQUIRED)
find_package(gflags REQUIRED)
include_directories(${GFLAGS_INCLUDE_DIR})
find_package(Glog REQUIRED)
include_directories(${GLOG_INCLUDE_DIRS})
find_package(folly CONFIG REQUIRED)
include_directories(${FOLLY_INCLUDE_DIR})
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
find_package(fb303 CONFIG REQUIRED)
find_package(fizz CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(wangle CONFIG REQUIRED)
find_package(FBThrift CONFIG REQUIRED)
include_directories(${FB303_INCLUDE_DIR})
find_path(FATAL_INCLUDE_DIR NAMES fatal/portability.h)
include_directories(${FATAL_INCLUDE_DIR})


find_library(NLOHMANN_JSON nlohmann-json)
find_path(NLOHMANN_JSON_INCLUDE_DIRS NAMES nlohmann/json.hpp)
include_directories(${NLOHMANN_JSON_INCLUDE_DIRS})

find_library(OPENNSA libopennsa.a opennsa)
find_path(OPENNSA_INCLUDE_DIR NAMES bcm/tx.h)
include_directories(${OPENNSA_INCLUDE_DIR})

message(STATUS "Found OPENNSA: ${OPENNSA}")
message(STATUS "Found OPENNSA_INCLUDE_DIR: ${OPENNSA_INCLUDE_DIR}")

find_package(Python3 COMPONENTS Interpreter Development)
include_directories(${Python3_INCLUDE_DIR})

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

find_library(YAML-CPP yaml-cpp)
find_path(YAML-CPP_INCLUDE_DIR NAMES yaml-cpp/yaml.h)
include_directories(${YAML-CPP_INCLUDE_DIR})
message(STATUS "Found YAML-CPP_INCLUDE_DIR: ${YAML-CPP_INCLUDE_DIR}")

find_library(YAML yaml)
find_path(YAML_INCLUDE_DIR NAMES yaml.h)
include_directories(${YAML_INCLUDE_DIR})
message(STATUS "Found YAML_INCLUDE_DIR: ${YAML_INCLUDE_DIR}")

# Generate thrift deps and libraries
include(FBThriftCppLibrary)

# Thrift libraries
add_fbthrift_cpp_library(
  fb303_cpp2
  common/fb303/if/fb303.thrift
  SERVICES
    FacebookService
  OPTIONS
    json
    reflection
)
add_fbthrift_cpp_library(
  network_address_cpp2
  common/network/if/Address.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_cpp_library(
  mpls_cpp2
  fboss/agent/if/mpls.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_cpp_library(
  switch_config_cpp2
  fboss/agent/switch_config.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    common_cpp2
    mpls_cpp2
)
add_fbthrift_cpp_library(
  agent_config_cpp2
  fboss/agent/agent_config.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    platform_config_cpp2
    switch_config_cpp2
)
add_fbthrift_cpp_library(
  platform_config_cpp2
  fboss/agent/platform_config.thrift
  OPTIONS
    json
    reflection
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
    reflection
  DEPENDS
    switch_config_cpp2
    phy_cpp2
    ctrl_cpp2
)
add_fbthrift_cpp_library(
  prbs_cpp2
  fboss/lib/phy/prbs.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_cpp_library(
  transceiver_cpp2
  fboss/qsfp_service/if/transceiver.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    prbs_cpp2
)
add_fbthrift_cpp_library(
  qsfp_config_cpp2
  fboss/qsfp_service/if/qsfp_service_config.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    transceiver_cpp2
)
add_fbthrift_cpp_library(
  qsfp_state_cpp2
  fboss/qsfp_service/if/qsfp_state.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    qsfp_config_cpp2
    phy_cpp2
    transceiver_cpp2
)
add_fbthrift_cpp_library(
  qsfp_stats_cpp2
  fboss/qsfp_service/if/qsfp_stats.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    hardware_stats_cpp2
    phy_cpp2
    transceiver_cpp2
)
add_fbthrift_cpp_library(
  sensor_service_stats_cpp2
  fboss/platform/sensor_service/sensor_service_stats.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    sensor_service_cpp2
)
add_fbthrift_cpp_library(
  phy_cpp2
  fboss/lib/phy/phy.thrift
  SERVICES
    FbossCommonPhyCtrl
  OPTIONS
    json
    reflection
  DEPENDS
    switch_config_cpp2
    transceiver_cpp2
    fboss_cpp2
    fb303_cpp2
)
add_fbthrift_cpp_library(
  optic_cpp2
  fboss/agent/if/optic.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    transceiver_cpp2
)
add_fbthrift_cpp_library(
  fboss_cpp2
  fboss/agent/if/fboss.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_cpp_library(
  sflow_cpp2
  fboss/agent/if/sflow.thrift
  OPTIONS
    json
)
add_fbthrift_cpp_library(
  common_cpp2
  fboss/agent/if/common.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    mpls_cpp2
    network_address_cpp2
)
add_fbthrift_cpp_library(
  product_info_cpp2
  fboss/agent/if/product_info.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_cpp_library(
  ctrl_cpp2
  fboss/agent/if/ctrl.thrift
  SERVICES
    FbossCtrl
    NeighborListenerClient
  OPTIONS
    json
    reflection
  DEPENDS
    fboss_cpp2
    fb303_cpp2
    mpls_cpp2
    network_address_cpp2
    optic_cpp2
    platform_config_cpp2
    product_info_cpp2
    transceiver_cpp2
    common_cpp2
    phy_cpp2
  OPTIONS
    json
)
add_fbthrift_cpp_library(
  packet_stream_cpp2
  fboss/agent/if/packet_stream.thrift
  SERVICES
    PacketStream
  DEPENDS
    fb303_cpp2
)
add_fbthrift_cpp_library(
  bcm_config_cpp2
  fboss/agent/hw/bcm/bcm_config.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_cpp_library(
  asic_config_cpp2
  fboss/agent/hw/sai/config/asic_config.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_cpp_library(
  packettrace_cpp2
  fboss/agent/hw/bcm/packettrace.thrift
)
add_fbthrift_cpp_library(
  mka_structs_cpp2
  fboss/mka_service/if/mka_structs.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_cpp_library(
  hardware_stats_cpp2
  fboss/agent/hw/hardware_stats.thrift
  OPTIONS
    json
    visitation
    reflection
  DEPENDS
    mka_structs_cpp2
)
add_fbthrift_cpp_library(
  agent_stats_cpp2
  fboss/agent/agent_stats.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    hardware_stats_cpp2
)
add_fbthrift_cpp_library(
  i2c_controller_stats_cpp2
  fboss/lib/i2c/i2c_controller_stats.thrift
)

add_fbthrift_cpp_library(
  led_mapping_cpp2
  fboss/lib/led/led_mapping.thrift
)

add_fbthrift_cpp_library(
  bsp_platform_mapping_cpp2
  fboss/lib/bsp/bsp_platform_mapping.thrift
  DEPENDS
    led_mapping_cpp2
)

add_fbthrift_cpp_library(
  qsfp_cpp2
  fboss/qsfp_service/if/qsfp.thrift
  SERVICES
    QsfpService
  OPTIONS
    json
  DEPENDS
    fb303_cpp2
    ctrl_cpp2
    fboss_cpp2
    transceiver_cpp2
    switch_config_cpp2
    mpls_cpp2
    mka_structs_cpp2
    hardware_stats_cpp2
)

add_fbthrift_cpp_library(
  sai_ctrl_cpp2
  fboss/agent/hw/sai/switch/sai_ctrl.thrift
  SERVICES
    SaiCtrl
  DEPENDS
    ctrl_cpp2
)

add_fbthrift_cpp_library(
  hw_test_ctrl_cpp2
  fboss/agent/hw/hw_test_ctrl.thrift
  SERVICES
    HwTestCtrl
  DEPENDS
    ctrl_cpp2
)

add_fbthrift_cpp_library(
  sai_test_ctrl_cpp2
  fboss/agent/hw/sai/hw_test/sai_test_ctrl.thrift
  SERVICES
    SaiTestCtrl
  DEPENDS
    ctrl_cpp2
    hw_test_ctrl_cpp2
)

find_library(USB usb-1.0)
find_path(USB_INCLUDE_DIR NAMES libusb-1.0/libusb.h)
include_directories(${USB_INCLUDE_DIR})

find_library(NETLINK3 nl-3)
find_library(NETLINKROUTE3 nl-route-3 libnl-route-3.a nl-route-3)

find_library(CURL curl)
find_path(CURL_INCLUDE_DIR NAMES curl/curl.h)
include_directories(${CURL_INCLUDE_DIR})

find_library(MNL mnl)
find_library(SODIUM sodium)

find_library(RE2 re2)
find_path(RE2_INCLUDE_DIR NAMES re2/re2.h)
include_directories(${RE2_INCLUDE_DIR})

find_library(EXPRTK exprtk)
find_path(EXPRTK_INCLUDE_DIR NAMES exprtk.hpp)
include_directories(${EXPRTK_INCLUDE_DIR})

# Unit Testing
add_definitions (-DIS_OSS=true)
find_package(Threads REQUIRED)
enable_testing()

if (NOT SAI_ONLY)
  add_executable(wedge_agent
      fboss/agent/platforms/wedge/WedgePlatform.cpp
      fboss/lib/platforms/PlatformProductInfo.cpp
      fboss/agent/platforms/wedge/WedgePort.cpp
      fboss/agent/platforms/wedge/wedge_agent.cpp
      fboss/agent/platforms/wedge/oss/WedgePlatform.cpp
  )

  add_library(transceiver_manager STATIC
      fboss/qsfp_service/TransceiverManager.cpp
      fboss/qsfp_service/TransceiverStateMachine.cpp
      fboss/qsfp_service/TransceiverStateMachineUpdate.cpp
  )

  add_library(qsfp_module STATIC
      fboss/qsfp_service/module/QsfpModule.cpp
      fboss/qsfp_service/module/oss/QsfpModule.cpp
      fboss/qsfp_service/module/QsfpUtil.cpp
      fboss/qsfp_service/module/sff/SffFieldInfo.cpp
      fboss/qsfp_service/module/sff/SffModule.cpp
      fboss/qsfp_service/module/sff/Sff8472Module.cpp
      fboss/qsfp_service/module/sff/Sff8472FieldInfo.cpp
      fboss/qsfp_service/module/oss/SffModule.cpp
      fboss/qsfp_service/module/cmis/CmisFieldInfo.cpp
      fboss/qsfp_service/module/cmis/CmisModule.cpp
  )

  add_executable(qsfp_service
      fboss/qsfp_service/QsfpServer.cpp
      fboss/qsfp_service/oss/QsfpServer.cpp
      fboss/qsfp_service/Main.cpp
      fboss/qsfp_service/QsfpServiceHandler.cpp
      fboss/qsfp_service/QsfpServiceSignalHandler.cpp
      fboss/qsfp_service/platforms/wedge/BspWedgeManager.cpp
      fboss/qsfp_service/platforms/wedge/WedgeManager.cpp
      fboss/qsfp_service/platforms/wedge/WedgeQsfp.cpp
      fboss/qsfp_service/platforms/wedge/Wedge100Manager.cpp
      fboss/qsfp_service/platforms/wedge/GalaxyManager.cpp
      fboss/qsfp_service/platforms/wedge/Wedge40Manager.cpp
      fboss/qsfp_service/platforms/wedge/WedgeManagerInit.cpp
      fboss/qsfp_service/platforms/wedge/oss/WedgeManagerInit.cpp
  )

  # TODO (skhare) refactor fboss_agent and remove this library
  add_library(switch_asics_old STATIC
      fboss/agent/hw/switch_asics/FakeAsic.h
      fboss/agent/hw/switch_asics/HwAsic.cpp
      fboss/agent/hw/switch_asics/HwAsic.h
      fboss/agent/hw/switch_asics/Tomahawk4Asic.cpp
      fboss/agent/hw/switch_asics/Tomahawk3Asic.cpp
      fboss/agent/hw/switch_asics/TomahawkAsic.cpp
      fboss/agent/hw/switch_asics/Trident2Asic.cpp
      fboss/agent/hw/switch_asics/IndusAsic.cpp
      fboss/agent/hw/switch_asics/BeasAsic.cpp
      fboss/agent/hw/switch_asics/BroadcomXgsAsic.cpp
  )

  add_library(fboss_agent STATIC
      fboss/agent/AclNexthopHandler.cpp
      fboss/agent/AgentConfig.cpp
      fboss/agent/AggregatePortStats.cpp
      fboss/agent/AlpmUtils.cpp
      fboss/agent/ApplyThriftConfig.cpp
      fboss/agent/ArpCache.cpp
      fboss/agent/ArpHandler.cpp
      fboss/agent/capture/PcapFile.cpp
      fboss/agent/capture/PcapPkt.cpp
      fboss/agent/capture/PcapQueue.cpp
      fboss/agent/capture/PcapWriter.cpp
      fboss/agent/capture/PktCapture.cpp
      fboss/agent/capture/PktCaptureManager.cpp
      fboss/agent/DHCPv4Handler.cpp
      fboss/agent/DHCPv6Handler.cpp
      fboss/agent/DsfSubscriber.cpp
      fboss/agent/EncapIndexAllocator.cpp
      fboss/agent/FibHelpers.cpp
      fboss/agent/FsdbSyncer.cpp
      fboss/agent/InterfaceStats.cpp
      fboss/agent/L2Entry.cpp
      fboss/agent/hw/BufferStatsLogger.cpp
      fboss/agent/hw/CounterUtils.cpp
      fboss/agent/hw/HwResourceStatsPublisher.cpp
      fboss/agent/hw/DiagCmdFilter.cpp
      fboss/agent/hw/HwSwitchWarmBootHelper.cpp
      fboss/agent/hw/HwSwitchStats.cpp
      fboss/agent/hw/HwTrunkCounters.cpp
      fboss/agent/hw/bcm/BcmAclEntry.cpp
      fboss/agent/hw/bcm/BcmAclStat.cpp
      fboss/agent/hw/bcm/BcmAclTable.cpp
      fboss/agent/hw/bcm/BcmAPI.cpp
      fboss/agent/hw/bcm/BcmBstStatsMgr.cpp
      fboss/agent/hw/bcm/BcmFacebookAPI.cpp
      fboss/agent/hw/bcm/BcmConfig.cpp
      fboss/agent/hw/bcm/BcmControlPlane.cpp
      fboss/agent/hw/bcm/BcmControlPlaneQueueManager.cpp
      fboss/agent/hw/bcm/BcmCosQueueManager.cpp
      fboss/agent/hw/bcm/BcmCosQueueFBConvertors.cpp
      fboss/agent/hw/bcm/BcmCosQueueManagerUtils.cpp
      fboss/agent/hw/bcm/BcmCosManager.cpp
      fboss/agent/hw/bcm/BcmEcmpUtils.cpp
      fboss/agent/hw/bcm/BcmEgress.cpp
      fboss/agent/hw/bcm/BcmEgressQueueFlexCounter.cpp
      fboss/agent/hw/bcm/BcmExactMatchUtils.cpp
      fboss/agent/hw/bcm/BcmFieldProcessorFBConvertors.cpp
      fboss/agent/hw/bcm/BcmFieldProcessorUtils.cpp
      fboss/agent/hw/bcm/BcmFlexCounter.cpp
      fboss/agent/hw/bcm/BcmFwLoader.cpp
      fboss/agent/hw/bcm/BcmHost.cpp
      fboss/agent/hw/bcm/BcmHostKey.cpp
      fboss/agent/hw/bcm/BcmIntf.cpp
      fboss/agent/hw/bcm/BcmIngressFieldProcessorFlexCounter.cpp
      fboss/agent/hw/bcm/BcmLabeledTunnel.cpp
      fboss/agent/hw/bcm/BcmLabelMap.cpp
      fboss/agent/hw/bcm/BcmLabelSwitchAction.cpp
      fboss/agent/hw/bcm/BcmLabelSwitchingUtils.cpp
      fboss/agent/hw/bcm/BcmLogBuffer.cpp
      fboss/agent/hw/bcm/BcmMirror.cpp
      fboss/agent/hw/bcm/BcmMirrorTable.cpp
      fboss/agent/hw/bcm/BcmMirrorUtils.cpp
      fboss/agent/hw/bcm/BcmPortDescriptor.cpp
      fboss/agent/hw/bcm/BcmPlatform.cpp
      fboss/agent/hw/bcm/BcmPlatformPort.cpp
      fboss/agent/hw/bcm/BcmPort.cpp
      fboss/agent/hw/bcm/BcmPortIngressBufferManager.cpp
      fboss/agent/hw/bcm/BcmPortUtils.cpp
      fboss/agent/hw/bcm/BcmPortGroup.cpp
      fboss/agent/hw/bcm/BcmPortQueueManager.cpp
      fboss/agent/hw/bcm/BcmPortResourceBuilder.cpp
      fboss/agent/hw/bcm/BcmPortTable.cpp
      fboss/agent/hw/bcm/BcmPrbs.cpp
      fboss/agent/hw/bcm/BcmPtpTcMgr.cpp
      fboss/agent/hw/bcm/BcmQcmCollector.cpp
      fboss/agent/hw/bcm/BcmQcmManager.cpp
      fboss/agent/hw/bcm/BcmQosMap.cpp
      fboss/agent/hw/bcm/BcmQosMapEntry.cpp
      fboss/agent/hw/bcm/BcmQosPolicy.cpp
      fboss/agent/hw/bcm/BcmQosPolicyTable.cpp
      fboss/agent/hw/bcm/BcmQosUtils.cpp
      fboss/agent/hw/bcm/BcmRoute.cpp
      fboss/agent/hw/bcm/BcmRouteCounter.cpp
      fboss/agent/hw/bcm/BcmRtag7LoadBalancer.cpp
      fboss/agent/hw/bcm/BcmRtag7Module.cpp
      fboss/agent/hw/bcm/BcmRxPacket.cpp
      fboss/agent/hw/bcm/BcmSflowExporter.cpp
      fboss/agent/hw/bcm/BcmStatUpdater.cpp
      fboss/agent/hw/bcm/BcmSwitch.cpp
      fboss/agent/hw/bcm/BcmSwitchEventCallback.cpp
      fboss/agent/hw/bcm/BcmSwitchEventUtils.cpp
      fboss/agent/hw/bcm/BcmTableStats.cpp
      fboss/agent/hw/bcm/BcmTeFlowEntry.cpp
      fboss/agent/hw/bcm/BcmTeFlowTable.cpp
      fboss/agent/hw/bcm/BcmTrunk.cpp
      fboss/agent/hw/bcm/BcmTrunkStats.cpp
      fboss/agent/hw/bcm/BcmTrunkTable.cpp
      fboss/agent/hw/bcm/BcmTxPacket.cpp
      fboss/agent/hw/bcm/BcmUnit.cpp
      fboss/agent/hw/bcm/BcmWarmBootCache.cpp
      fboss/agent/hw/bcm/BcmWarmBootHelper.cpp
      fboss/agent/hw/bcm/BcmWarmBootState.cpp
      fboss/agent/hw/bcm/PortAndEgressIdsMap.cpp
      fboss/agent/hw/bcm/BcmEgressManager.cpp
      fboss/agent/hw/bcm/BcmNextHop.cpp
      fboss/agent/hw/bcm/BcmMultiPathNextHop.cpp
      fboss/agent/hw/bcm/BcmLabeledEgress.cpp
      fboss/agent/hw/bcm/BcmLabeledTunnelEgress.cpp
      fboss/agent/hw/bcm/BcmClassIDUtil.cpp
      fboss/agent/hw/bcm/BcmSwitchSettings.cpp
      fboss/agent/hw/bcm/BcmTypesImpl.cpp
      fboss/agent/hw/bcm/BcmMacTable.cpp
      fboss/agent/hw/bcm/PacketTraceUtils.cpp
      fboss/agent/hw/bcm/RxUtils.cpp
      fboss/agent/hw/bcm/oss/BcmAPI.cpp
      fboss/agent/hw/bcm/oss/BcmFacebookAPI.cpp
      fboss/agent/hw/bcm/oss/BcmPort.cpp
      fboss/agent/hw/bcm/oss/BcmQcmCollector.cpp
      fboss/agent/hw/bcm/oss/BcmQcmManager.cpp
      fboss/agent/hw/bcm/oss/BcmSwitch.cpp
      fboss/agent/hw/bcm/oss/BcmBstStatsMgr.cpp
      fboss/agent/hw/bcm/oss/BcmUnit.cpp
      fboss/agent/hw/bcm/oss/BcmTableStats.cpp
      fboss/agent/hw/bcm/BcmAddressFBConvertors.cpp
      fboss/agent/hw/mock/MockHwSwitch.cpp
      fboss/agent/hw/mock/MockPlatform.cpp
      fboss/agent/hw/mock/MockRxPacket.cpp
      fboss/agent/hw/mock/MockTxPacket.cpp
      fboss/agent/hw/mock/MockTestHandle.cpp
      fboss/agent/hw/sim/SimSwitch.cpp
      fboss/agent/ndp/IPv6RouteAdvertiser.cpp
      fboss/agent/HwSwitch.cpp
      fboss/agent/IPHeaderV4.cpp
      fboss/agent/IPv4Handler.cpp
      fboss/agent/IPv6Handler.cpp
      fboss/agent/LacpController.cpp
      fboss/agent/LacpMachines.cpp
      fboss/agent/LacpTypes.cpp
      fboss/agent/LinkAggregationManager.cpp
      fboss/agent/LldpManager.cpp
      fboss/agent/LoadBalancerConfigApplier.cpp
      fboss/agent/Main.cpp
      fboss/agent/MPLSHandler.cpp
      fboss/agent/SetupThrift.cpp
      fboss/agent/MKAServicePorts.cpp
      fboss/agent/MKAServiceManager.cpp
      fboss/agent/MirrorManager.cpp
      fboss/agent/MirrorManagerImpl.cpp
      fboss/agent/LookupClassRouteUpdater.cpp
      fboss/agent/LookupClassUpdater.cpp
      fboss/agent/MacTableManager.cpp
      fboss/agent/MacTableUtils.cpp
      fboss/agent/ResolvedNexthopMonitor.cpp
      fboss/agent/ResolvedNexthopProbe.cpp
      fboss/agent/ResolvedNexthopProbeScheduler.cpp
      fboss/agent/ndp/IPv6RouteAdvertiser.cpp
      fboss/agent/NdpCache.cpp
      fboss/agent/NeighborUpdater.cpp
      fboss/agent/NeighborUpdaterImpl.cpp
      fboss/agent/NeighborUpdaterNoopImpl.cpp
      fboss/agent/normalization/Normalizer.cpp
      fboss/agent/normalization/oss/Normalizer.cpp
      fboss/agent/normalization/PortStatsProcessor.cpp
      fboss/agent/normalization/TransformHandler.cpp
      fboss/agent/normalization/StatsExporter.cpp
      fboss/agent/normalization/CounterTagManager.cpp
      fboss/agent/oss/DsfSubscriber.cpp
      fboss/agent/oss/AggregatePortStats.cpp
      fboss/agent/oss/FbossInit.cpp
      fboss/agent/oss/FsdbStateDeltaConverter.cpp
      fboss/agent/oss/FsdbSyncer.cpp
      fboss/agent/oss/Main.cpp
      fboss/agent/oss/PacketLogger.cpp
      fboss/agent/oss/SetupThrift.cpp
      fboss/agent/oss/RouteUpdateLogger.cpp
      fboss/agent/oss/SwSwitch.cpp
      fboss/agent/oss/Utils.cpp
      fboss/agent/PacketObserver.cpp
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
      fboss/agent/packet/PTPHeader.cpp
      fboss/agent/packet/UDPHeader.cpp
      fboss/agent/packet/MPLSHdr.cpp
      fboss/agent/Platform.cpp
      fboss/agent/PlatformPort.cpp
      fboss/lib/platforms/PlatformProductInfo.cpp
      fboss/lib/platforms/oss/PlatformProductInfo.cpp
      fboss/agent/platforms/wedge/wedge_agent.cpp
      fboss/agent/platforms/wedge/WedgePlatform.cpp
      fboss/agent/platforms/wedge/WedgePort.cpp
      fboss/agent/platforms/wedge/WedgeTomahawkPlatform.cpp
      fboss/agent/platforms/wedge/WedgePlatformInit.cpp
      fboss/agent/platforms/wedge/galaxy/GalaxyPort.cpp
      fboss/agent/platforms/wedge/galaxy/GalaxyLCPlatform.cpp
      fboss/agent/platforms/wedge/galaxy/GalaxyFCPlatform.cpp
      fboss/agent/platforms/wedge/galaxy/oss/GalaxyPlatform.cpp
      fboss/agent/platforms/wedge/galaxy/oss/GalaxyPort.cpp
      fboss/agent/platforms/wedge/oss/WedgePlatform.cpp
      fboss/agent/platforms/wedge/oss/WedgePlatformInit.cpp
      fboss/agent/platforms/wedge/wedge100/Wedge100Platform.cpp
      fboss/agent/platforms/wedge/wedge100/Wedge100Port.cpp
      fboss/agent/platforms/wedge/wedge100/oss/Wedge100Port.cpp
      fboss/agent/platforms/wedge/wedge40/FakeWedge40Platform.cpp
      fboss/agent/platforms/wedge/wedge40/Wedge40Platform.cpp
      fboss/agent/platforms/wedge/wedge40/Wedge40Port.cpp
      fboss/agent/platforms/wedge/wedge40/oss/Wedge40Port.cpp
      fboss/agent/PortStats.cpp
      fboss/agent/PortUpdateHandler.cpp
      fboss/agent/RouteUpdateLogger.cpp
      fboss/agent/RouteUpdateLoggingPrefixTracker.cpp
      fboss/agent/StaticL2ForNeighborObserver.cpp
      fboss/agent/StaticL2ForNeighborUpdater.cpp
      fboss/agent/StaticL2ForNeighborSwSwitchUpdater.cpp
      fboss/agent/state/AclEntry.cpp
      fboss/agent/state/AclMap.cpp
      fboss/agent/state/AclTable.cpp
      fboss/agent/state/AclTableGroup.cpp
      fboss/agent/state/AclTableGroupMap.cpp
      fboss/agent/state/AclTableMap.cpp
      fboss/agent/state/AggregatePort.cpp
      fboss/agent/state/AggregatePortMap.cpp
      fboss/agent/state/ArpEntry.cpp
      fboss/agent/state/ArpResponseEntry.cpp
      fboss/agent/state/ArpResponseTable.cpp
      fboss/agent/state/ArpTable.cpp
      fboss/agent/state/ControlPlane.cpp
      fboss/agent/state/DsfNode.cpp
      fboss/agent/state/DsfNodeMap.cpp
      fboss/agent/state/UdfConfig.cpp
      fboss/agent/state/UdfGroup.cpp
      fboss/agent/state/UdfGroupMap.cpp
      fboss/agent/state/UdfPacketMatcher.cpp
      fboss/agent/state/UdfPacketMatcherMap.cpp
      fboss/agent/state/ForwardingInformationBase.cpp
      fboss/agent/state/ForwardingInformationBaseContainer.cpp
      fboss/agent/state/ForwardingInformationBaseDelta.cpp
      fboss/agent/state/ForwardingInformationBaseMap.cpp
      fboss/agent/state/Interface.cpp
      fboss/agent/state/InterfaceMap.cpp
      fboss/agent/state/InterfaceMapDelta.cpp
      fboss/agent/state/IpTunnel.cpp
      fboss/agent/state/IpTunnelMap.cpp
      fboss/agent/state/LabelForwardingAction.cpp
      fboss/agent/state/LabelForwardingInformationBase.cpp
      fboss/agent/state/LoadBalancer.cpp
      fboss/agent/state/LoadBalancerMap.cpp
      fboss/agent/state/MacEntry.cpp
      fboss/agent/state/MacTable.cpp
      fboss/agent/state/Mirror.cpp
      fboss/agent/state/MirrorMap.cpp
      fboss/agent/state/MatchAction.cpp
      fboss/agent/state/NdpEntry.cpp
      fboss/agent/state/NdpResponseEntry.cpp
      fboss/agent/state/NdpResponseTable.cpp
      fboss/agent/state/NdpTable.cpp
      fboss/agent/state/NodeBase.cpp
      fboss/agent/state/Port.cpp
      fboss/agent/state/PortMap.cpp
      fboss/agent/state/PortQueue.cpp
      fboss/agent/state/PortPgConfig.cpp
      fboss/agent/state/BufferPoolConfig.cpp
      fboss/agent/state/BufferPoolConfigMap.cpp
      fboss/agent/state/QosPolicy.cpp
      fboss/agent/state/QosPolicyMap.cpp
      fboss/agent/state/Route.cpp
      fboss/agent/state/RouteNextHop.cpp
      fboss/agent/state/RouteNextHopEntry.cpp
      fboss/agent/state/RouteNextHopsMulti.cpp
      fboss/agent/state/RouteTypes.cpp
      fboss/agent/state/SflowCollector.cpp
      fboss/agent/state/SflowCollectorMap.cpp
      fboss/agent/state/StateDelta.cpp
      fboss/agent/state/StateUtils.cpp
      fboss/agent/state/SwitchState.cpp
      fboss/agent/state/SystemPort.cpp
      fboss/agent/state/SystemPortMap.cpp
      fboss/agent/state/TeFlowEntry.cpp
      fboss/agent/state/TeFlowTable.cpp
      fboss/agent/state/Transceiver.cpp
      fboss/agent/state/TransceiverMap.cpp
      fboss/agent/state/Vlan.cpp
      fboss/agent/state/VlanMap.cpp
      fboss/agent/state/VlanMapDelta.cpp
      fboss/agent/state/SwitchSettings.cpp
      fboss/agent/state/QcmConfig.cpp
      fboss/agent/types.cpp
      fboss/agent/RouteUpdateWrapper.cpp
      fboss/agent/RestartTimeTracker.cpp
      fboss/agent/SwitchStats.cpp
      fboss/agent/SwSwitch.cpp
      fboss/agent/SwSwitchRouteUpdateWrapper.cpp
      fboss/agent/TeFlowNexthopHandler.cpp
      fboss/agent/ThriftHandler.cpp
      fboss/agent/TunIntf.cpp
      fboss/agent/TunManager.cpp
      fboss/agent/Utils.cpp
      fboss/agent/rib/ConfigApplier.cpp
      fboss/agent/rib/ForwardingInformationBaseUpdater.cpp
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
      fboss/lib/firmware_storage/FbossFirmware.cpp
      fboss/lib/firmware_storage/FbossFirmware.h
      fboss/lib/firmware_storage/FbossFwStorage.cpp
      fboss/lib/firmware_storage/FbossFwStorage.h
      fboss/lib/i2c/PCA9541.cpp
      fboss/lib/i2c/PCA9541.h
      fboss/lib/i2c/FirmwareUpgrader.cpp
      fboss/lib/i2c/FirmwareUpgrader.h
      fboss/lib/i2c/CdbCommandBlock.cpp
      fboss/lib/i2c/CdbCommandBlock.h
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
      fboss/lib/oss/LogThriftCall.cpp
      fboss/lib/AlertLogger.cpp
  )

  target_link_libraries(
    fboss_agent PUBLIC
      agent_config_cpp2
      agent_stats_cpp2
      switch_asics_old
      switch_config_cpp2
      switch_state_cpp2
      sflow_cpp2
      ctrl_cpp2
      packettrace_cpp2
      hardware_stats_cpp2
      i2c_controller_stats_cpp2
      mpls_cpp2
      qsfp_cpp2
      lldp
      async_logger
      common_utils
      common_file_utils
      Folly::folly
      wangle::wangle
      fb303::fb303
      FBThrift::thriftcpp2
      ${USB}
      ${IPROUTE2}
      ${YAML-CPP}
      ${NETLINK3}
      ${NETLINKROUTE3}
      ${CURL}
      ${SODIUM}
      ${MNL}
      ${OPENNSA}
      ${YAML}
      wedge40_platform_mapping
      wedge400c_platform_mapping
      wedge100_platform_mapping
      galaxy_platform_mapping
      kamet_platform_mapping
      makalu_platform_mapping
      qsfp_cpp2
      qsfp_service_client
      wedge_led_utils
      bcm_yaml_config
      bidirectional_packet_stream
      snapshot_manager
      fsdb_stream_client
      fsdb_pub_sub
      fsdb_flags
      qsfp_lib
      thread_heartbeat
      phy_utils
  )

  set_target_properties(fboss_agent PROPERTIES COMPILE_FLAGS
    "-DINCLUDE_L3 -DBCM_ESW_SUPPORT"
  )

  target_link_libraries(transceiver_manager
    fboss_error
    fboss_types
    qsfp_config
    phy_management_base
    thrift_service_client
    common_file_utils
  )

  target_link_libraries(qsfp_module
    fboss_i2c_lib
    qsfp_config_parser
    snapshot_manager
  )

  target_link_libraries(switch_asics_old fboss_agent)
  target_link_libraries(wedge_agent fboss_agent)
  target_link_libraries(qsfp_service
      qsfp_module
      fboss_agent
      qsfp_config
      phy_management_base
      transceiver_manager
  )

  add_executable(cp2112_util
      fboss/util/cp2112_util.cpp
  )
  target_link_libraries(cp2112_util fboss_agent)

  add_executable(wedge_qsfp_util
      fboss/qsfp_service/platforms/wedge/BspWedgeManager.cpp
      fboss/qsfp_service/platforms/wedge/WedgeManager.cpp
      fboss/qsfp_service/platforms/wedge/Wedge100Manager.cpp
      fboss/qsfp_service/platforms/wedge/GalaxyManager.cpp
      fboss/qsfp_service/platforms/wedge/Wedge40Manager.cpp
      fboss/qsfp_service/platforms/wedge/WedgeManagerInit.cpp
      fboss/qsfp_service/platforms/wedge/oss/WedgeManagerInit.cpp
      fboss/util/wedge_qsfp_util.cpp
      fboss/util/qsfp_util_main.cpp
      fboss/util/qsfp/QsfpUtilTx.cpp
      fboss/util/qsfp/QsfpServiceDetector.cpp
      fboss/util/oss/wedge_qsfp_util.cpp
      fboss/lib/firmware_storage/FbossFirmware.cpp
      fboss/lib/firmware_storage/FbossFirmware.h
      fboss/lib/firmware_storage/FbossFwStorage.cpp
      fboss/lib/firmware_storage/FbossFwStorage.h
      fboss/lib/i2c/CdbCommandBlock.h
      fboss/lib/i2c/CdbCommandBlock.cpp
      fboss/lib/i2c/FirmwareUpgrader.cpp
      fboss/lib/i2c/FirmwareUpgrader.h
  )
  target_link_libraries(wedge_qsfp_util
      fboss_agent
      qsfp_config
      qsfp_lib
      qsfp_module
      ${YAML-CPP}
      phy_management_base
      transceiver_manager
  )

  # Don't include fboss/agent/test/ArpBenchmark.cpp
  # It depends on the Sim implementation and needs its own target
  add_executable(agent_test
         fboss/agent/test/TestUtils.cpp
         fboss/agent/test/ArpTest.cpp
         fboss/agent/test/CounterCache.cpp
         fboss/agent/test/DHCPv4HandlerTest.cpp
         fboss/agent/test/EcmpSetupHelper.cpp
         fboss/agent/test/FibHelperTests.cpp
         fboss/agent/test/ICMPTest.cpp
         fboss/agent/test/IPv4Test.cpp
         fboss/agent/test/LldpManagerTest.cpp
         fboss/agent/test/LabelForwardingUtils.cpp
         fboss/agent/test/LookupClassRouteUpdaterTests.cpp
         fboss/agent/test/LookupClassUpdaterTests.cpp
         fboss/agent/test/MKAServiceManagerTest.cpp
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
         fboss/agent/test/RouteUpdateLoggerTest.cpp
         fboss/agent/test/ResourceLibUtilTest.cpp
         fboss/agent/test/RouteDistributionGeneratorTest.cpp
         fboss/agent/test/RouteScaleGeneratorsTest.cpp
         fboss/agent/test/StaticL2ForNeighborObserverTests.cpp
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
      ${GTEST_INCLUDE_DIRS}
  )

  target_link_libraries(agent_test
      fboss_agent
      ${GTEST}
      ${CMAKE_THREAD_LIBS_INIT}
      ${LIBGMOCK_LIBRARIES}
  )
  gtest_discover_tests(agent_test)

  #TODO: Add tests from other folders aside from agent/test

  install(TARGETS wedge_agent)

endif()

# Build all cmake files under cmake/*cmake
file(GLOB cmakeFiles ${CMAKE_CURRENT_SOURCE_DIR}/cmake/*.cmake)
foreach(cmakeFile ${cmakeFiles})
  if(SAI_ONLY AND ${cmakeFile} MATCHES "Bcm")
    message(status " Skipping OpenNSA BCM binaries")
  # Skip building LinkTest, enable this for non SAI_ONLY build
  # once the compilation issues with LinkTest are fixed
  elseif(${cmakeFile} MATCHES "LinkTest")
    message(status " Skipping LinkTest binaries")
  else()
    message(status " INCLUDE ${cmakeFile}")
    INCLUDE(${cmakeFile})
  endif()
endforeach(cmakeFile)

if (NOT SAI_ONLY)
  install(TARGETS bcm_test)
endif()

install(TARGETS fake_sai)

install(
  TARGETS
  sai_test-fake-${SAI_VER_SUFFIX})

install(
  TARGETS
  sai_replayer-fake-${SAI_VER_SUFFIX})
