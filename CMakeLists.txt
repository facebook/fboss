cmake_minimum_required(VERSION 2.8)
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
set(CMAKE_PROGRAM_PATH "${CMAKE_SOURCE_DIR}/external/fbthrift/thrift/compiler/;${CMAKE_PROGRAM_PATH}")
set(PCAP_DIR_PATH
  "${CMAKE_BINARY_DIR}/gen/fboss/pcap_distribution_service/if/gen-cpp2/")

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR}/gen)
include_directories(${CMAKE_BINARY_DIR}/gen/fboss/agent/gen-cpp2)
include_directories(${CMAKE_BINARY_DIR}/gen/fboss/agent/if/gen-cpp2)
include_directories(${CMAKE_BINARY_DIR}/gen/common/fb303/if/gen-cpp2)
include_directories(${CMAKE_BINARY_DIR}/gen/common/network/if/gen-cpp2)
include_directories(${CMAKE_BINARY_DIR}/gen/fboss/qsfp_service/if/gen-cpp2)

# System libraries
find_library(GLOG glog)
find_library(BOOSTSER boost_serialization)
find_library(BOOSTFS boost_filesystem)
find_library(BOOSTSYS boost_system)
find_library(BOOSTTHREAD boost_thread)
find_library(DOUBLECONV double-conversion)
find_library(GFLAGS gflags)
find_library(GLOG glog)
find_library(PTHREAD pthread)
find_library(USB usb-1.0)
find_library(PCAP pcap)
find_library(SNAPPY snappy)
find_library(ZLIB z)
find_library(SSL ssl)
find_library(CRYPTO crypto)
find_library(DL dl)
find_library(EVENT event)
find_library(NETLINK3 nl-3)
find_library(NETLINKROUTE3 nl-route-3)
find_library(NUMA numa)
find_library(CURL curl)

# External libraries that are not generally available. Look in external/
# for these. This is where getdeps.sh will toss them.
find_library(FOLLY folly PATHS ${CMAKE_SOURCE_DIR}/external/folly/folly/.libs)
find_library(WANGLE wangle PATHS ${CMAKE_SOURCE_DIR}/external/wangle/wangle/build/lib)
find_library(THRIFT thrift PATHS ${CMAKE_SOURCE_DIR}/external/fbthrift/thrift/lib/cpp/.libs)
find_library(THRIFTPROTO thriftprotocol PATHS ${CMAKE_SOURCE_DIR}/external/fbthrift/thrift/lib/cpp2/.libs)
find_library(THRIFTCPP2 thriftcpp2 PATHS ${CMAKE_SOURCE_DIR}/external/fbthrift/thrift/lib/cpp2/.libs)
find_library(OPENNSL opennsl PATHS ${CMAKE_SOURCE_DIR}/external/OpenNSL/bin/wedge-trident)
find_library(IPROUTE2 netlink PATHS ${CMAKE_SOURCE_DIR}/external/iproute2/lib)
find_library(ZSTD zstd PATHS ${CMAKE_SOURCE_DIR}/external/zstd/lib)

include_directories(${CMAKE_SOURCE_DIR}/external/OpenNSL/include)
include_directories(${CMAKE_SOURCE_DIR}/external/iproute2/include)
include_directories(${CMAKE_SOURCE_DIR}/external/folly)
include_directories(${CMAKE_SOURCE_DIR}/external/fbthrift)
include_directories(${CMAKE_SOURCE_DIR}/external/wangle)

add_executable(wedge_agent
    fboss/agent/platforms/wedge/WedgePlatform.cpp
    fboss/agent/platforms/wedge/WedgeProductInfo.cpp
    fboss/agent/platforms/wedge/WedgePort.cpp
    fboss/agent/platforms/wedge/wedge_ctrl.cpp
    fboss/agent/platforms/wedge/oss/WedgePort.cpp
    fboss/agent/platforms/wedge/oss/WedgePlatform.cpp
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
)

target_link_libraries(wedge_agent fboss_agent)
target_link_libraries(qsfp_service fboss_agent)

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
    fboss/agent/hw/bcm/BcmAclTable.cpp
    fboss/agent/hw/bcm/BcmAPI.cpp
    fboss/agent/hw/bcm/BcmEgress.cpp
    fboss/agent/hw/bcm/BcmHost.cpp
    fboss/agent/hw/bcm/BcmIntf.cpp
    fboss/agent/hw/bcm/BcmPlatform.cpp
    fboss/agent/hw/bcm/BcmPort.cpp
    fboss/agent/hw/bcm/BcmPortGroup.cpp
    fboss/agent/hw/bcm/BcmPortTable.cpp
    fboss/agent/hw/bcm/BcmRoute.cpp
    fboss/agent/hw/bcm/BcmRxPacket.cpp
    fboss/agent/hw/bcm/BcmSflowExporter.cpp
    fboss/agent/hw/bcm/BcmStats.cpp
    fboss/agent/hw/bcm/BcmSwitch.cpp
    fboss/agent/hw/bcm/BcmSwitchEventCallback.cpp
    fboss/agent/hw/bcm/BcmSwitchEventUtils.cpp
    fboss/agent/hw/bcm/BcmTrunkTable.cpp
    fboss/agent/hw/bcm/BcmTxPacket.cpp
    fboss/agent/hw/bcm/BcmWarmBootCache.cpp
    fboss/agent/hw/bcm/BcmWarmBootHelper.cpp
    fboss/agent/hw/bcm/PortAndEgressIdsMap.cpp
    fboss/agent/hw/bcm/oss/BcmAclEntry.cpp
    fboss/agent/hw/bcm/oss/BcmAPI.cpp
    fboss/agent/hw/bcm/oss/BcmEgress.cpp
    fboss/agent/hw/bcm/oss/BcmHost.cpp
    fboss/agent/hw/bcm/oss/BcmPort.cpp
    fboss/agent/hw/bcm/oss/BcmPortGroup.cpp
    fboss/agent/hw/bcm/oss/BcmPortTable.cpp
    fboss/agent/hw/bcm/oss/BcmSwitch.cpp
    fboss/agent/hw/bcm/oss/BcmSwitchEventCallback.cpp
    fboss/agent/hw/bcm/oss/BcmTrunk.cpp
    fboss/agent/hw/bcm/oss/BcmUnit.cpp
    fboss/agent/hw/bcm/oss/BcmWarmBootHelper.cpp
    fboss/agent/hw/bcm/oss/BcmTableStats.cpp
    fboss/agent/hw/bcm/Utils.cpp
    fboss/agent/hw/mock/MockRxPacket.cpp
    fboss/agent/hw/mock/MockTxPacket.cpp
    fboss/agent/hw/sim/SimHandler.cpp
    fboss/agent/hw/sim/SimPlatform.cpp
    fboss/agent/hw/sim/SimSwitch.cpp
    fboss/agent/lldp/LinkNeighbor.cpp
    fboss/agent/lldp/LinkNeighborDB.cpp
    fboss/agent/ndp/IPv6RouteAdvertiser.cpp
    fboss/agent/oss/ApplyThriftConfig.cpp
    fboss/agent/oss/Main.cpp
    fboss/agent/oss/RouteUpdateLogger.cpp
    fboss/agent/oss/SwSwitch.cpp
    fboss/agent/HwSwitch.cpp
    fboss/agent/I2c.cpp
    fboss/agent/IPHeaderV4.cpp
    fboss/agent/IPv4Handler.cpp
    fboss/agent/IPv6Handler.cpp
    fboss/agent/lldp/LinkNeighbor.cpp
    fboss/agent/lldp/LinkNeighborDB.cpp
    fboss/agent/LldpManager.cpp
    fboss/agent/Main.cpp
    fboss/agent/ndp/IPv6RouteAdvertiser.cpp
    fboss/agent/NdpCache.cpp
    fboss/agent/NeighborListenerClient.cpp
    fboss/agent/NeighborUpdater.cpp
    fboss/agent/NexthopToRouteCount.cpp
    fboss/agent/oss/ApplyThriftConfig.cpp
    fboss/agent/oss/Main.cpp
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
    fboss/agent/platforms/wedge/oss/WedgeProductInfo.cpp
    fboss/agent/platforms/wedge/wedge_ctrl.cpp
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
    fboss/agent/PortRemediator.cpp
    fboss/agent/QsfpClient.cpp
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
    fboss/agent/state/Interface.cpp
    fboss/agent/state/InterfaceMap.cpp
    fboss/agent/state/MatchToActionEntry.cpp
    fboss/agent/state/NdpEntry.cpp
    fboss/agent/state/NdpResponseTable.cpp
    fboss/agent/state/NdpTable.cpp
    fboss/agent/state/NeighborResponseTable.cpp
    fboss/agent/state/NodeBase.cpp
    fboss/agent/state/Port.cpp
    fboss/agent/state/PortMap.cpp
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
    fboss/agent/state/TrafficPolicy.cpp
    fboss/agent/state/Vlan.cpp
    fboss/agent/state/VlanMap.cpp
    fboss/agent/state/VlanMapDelta.cpp
    fboss/agent/SwitchStats.cpp
    fboss/agent/SwSwitch.cpp
    fboss/agent/ThriftHandler.cpp
    fboss/agent/ThreadHeartbeat.cpp
    fboss/agent/TunIntf.cpp
    fboss/agent/TunManager.cpp
    fboss/agent/UDPHeader.cpp
    fboss/agent/UnresolvedNhopsProber.cpp
    fboss/agent/Utils.cpp

    fboss/lib/usb/GalaxyI2CBus.cpp
    fboss/lib/usb/BaseWedgeI2CBus.cpp
    fboss/lib/usb/BaseWedgeI2CBus.h
    fboss/lib/usb/CP2112.cpp
    fboss/lib/usb/CP2112.h
    fboss/lib/usb/PCA9548MultiplexedBus.cpp
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



    # generated sources
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/hw/bcm/gen-cpp2/packettrace_data.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/hw/bcm/gen-cpp2/packettrace_types.cpp
    ${CMAKE_BINARY_DIR}/gen/common/fb303/if/gen-cpp2/FacebookService.cpp
    ${CMAKE_BINARY_DIR}/gen/common/fb303/if/gen-cpp2/FacebookService_client.cpp
    ${CMAKE_BINARY_DIR}/gen/common/fb303/if/gen-cpp2/fb303_data.cpp
    ${CMAKE_BINARY_DIR}/gen/common/fb303/if/gen-cpp2/fb303_types.cpp
    ${CMAKE_BINARY_DIR}/gen/common/network/if/gen-cpp/Address_constants.cpp
    ${CMAKE_BINARY_DIR}/gen/common/network/if/gen-cpp/Address_reflection.cpp
    ${CMAKE_BINARY_DIR}/gen/common/network/if/gen-cpp/Address_data.cpp
    ${CMAKE_BINARY_DIR}/gen/common/network/if/gen-cpp/Address_types.cpp
    ${CMAKE_BINARY_DIR}/gen/common/network/if/gen-cpp2/Address_constants.cpp
    ${CMAKE_BINARY_DIR}/gen/common/network/if/gen-cpp2/Address_data.cpp
    ${CMAKE_BINARY_DIR}/gen/common/network/if/gen-cpp2/Address_types.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/gen-cpp2/switch_config_constants.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/gen-cpp2/switch_config_data.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/gen-cpp2/switch_config_types.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/hw/sim/gen-cpp2/SimCtrl.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/hw/sim/gen-cpp2/sim_ctrl_data.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/hw/sim/gen-cpp2/sim_ctrl_types.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/hw/sim/gen-cpp2/sim_ctrl_constants.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/if/gen-cpp2/optic_data.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/if/gen-cpp2/optic_types.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/if/gen-cpp2/sflow_data.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/if/gen-cpp2/sflow_types.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/if/gen-cpp2/sflow_constants.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/if/gen-cpp2/ctrl_data.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/if/gen-cpp2/ctrl_types.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/if/gen-cpp2/FbossCtrl.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/if/gen-cpp2/NeighborListenerClient_client.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/if/gen-cpp2/FbossHighresClient_client.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/if/gen-cpp2/fboss_data.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/if/gen-cpp2/fboss_types.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/if/gen-cpp2/highres_data.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/if/gen-cpp2/highres_types.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/hw/bcm/gen-cpp2/bcmswitch_data.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/hw/bcm/gen-cpp2/bcmswitch_types.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/hw/bcm/gen-cpp2/hardware_stats_data.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/hw/bcm/gen-cpp2/hardware_stats_types.cpp
    ${PCAP_DIR_PATH}PcapPushSubscriber.cpp
    ${PCAP_DIR_PATH}PcapPushSubscriber_client.cpp
    ${PCAP_DIR_PATH}pcap_pubsub_constants.cpp
    ${PCAP_DIR_PATH}pcap_pubsub_types.cpp
    ${PCAP_DIR_PATH}pcap_pubsub_data.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/qsfp_service/if/gen-cpp2/QsfpService.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/qsfp_service/if/gen-cpp2/QsfpService_client.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/qsfp_service/if/gen-cpp2/qsfp_types.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/qsfp_service/if/gen-cpp2/qsfp_data.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/qsfp_service/if/gen-cpp2/qsfp_constants.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/qsfp_service/if/gen-cpp2/transceiver_types.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/qsfp_service/if/gen-cpp2/transceiver_data.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/qsfp_service/if/gen-cpp2/transceiver_constants.cpp
)

target_link_libraries(fboss_agent
    ${GLOG}
    ${SSL}
    ${PCAP}
    ${USB}
    ${ZLIB}
    ${SNAPPY}
    ${CRYPTO}
    ${FOLLY}
    ${WANGLE}
    ${GFLAGS}
    ${GLOG}
    ${PTHREAD}
    ${THRIFT}
    ${THRIFTPROTO}
    ${THRIFTCPP2}
    ${OPENNSL}
    ${IPROUTE2}
    ${BOOSTFS}
    ${BOOSTSER}
    ${BOOSTSYS}
    ${BOOSTTHREAD}
    ${DOUBLECONV}
    ${DL}
    ${EVENT}
    ${NETLINK3}
    ${NETLINKROUTE3}
    ${CURL}
    ${NUMA}
    ${ZSTD}
)

target_link_libraries(qsfp_service
    ${ZSTD}
    ${GLOG}
    ${SSL}
    ${ZLIB}
    ${FOLLY}
    ${WANGLE}
    ${GFLAGS}
    ${GLOG}
    ${PTHREAD}
    ${THRIFT}
    ${THRIFTPROTO}
    ${THRIFTCPP2}
    ${EVENT}
)

find_program(THRIFT1 thrift1)
find_program(PYTHON python)
set(THRIFTC2OPTS json)
set(THRIFTC ${THRIFT1} -I ${CMAKE_SOURCE_DIR} -gen cpp:json,templates -gen py:)
set(THRIFTC2
    ${PYTHON} -mthrift_compiler.main -I ${CMAKE_SOURCE_DIR} --gen cpp2:)

function(fboss_add_thrift)
    cmake_parse_arguments("arg" "" "OPTIONS;THRIFTSRC;REFLECT" "SERVICES" ${ARGN})
    get_cmake_property(_variableNames VARIABLES)
    get_filename_component(dirname "${arg_THRIFTSRC}" PATH)
    # NAME_WE = name without extension.
    get_filename_component(basename "${arg_THRIFTSRC}" NAME_WE)
    foreach(srv ${arg_SERVICES})
        set(services ${services} ${CMAKE_BINARY_DIR}/gen/${dirname}/gen-cpp/${srv}.cpp)
        set(services ${services} ${CMAKE_BINARY_DIR}/gen/${dirname}/gen-cpp/${srv}.h)
        set(services ${services} ${CMAKE_BINARY_DIR}/gen/${dirname}/gen-cpp2/${srv}_client.cpp)
        set(services ${services} ${CMAKE_BINARY_DIR}/gen/${dirname}/gen-cpp2/${srv}.cpp)
        set(services ${services} ${CMAKE_BINARY_DIR}/gen/${dirname}/gen-cpp2/${srv}.h)
    endforeach(srv)
    foreach(reflect ${arg_REFLECT})
        set(outputs ${outputs}
            ${CMAKE_BINARY_DIR}/gen/${dirname}/gen-cpp/${reflect}_reflection.h
            ${CMAKE_BINARY_DIR}/gen/${dirname}/gen-cpp/${reflect}_reflection.cpp)
    endforeach(reflect)
    foreach(gen cpp cpp2)
        foreach(output constants data types)
            set(outputs ${outputs}
                ${CMAKE_BINARY_DIR}/gen/${dirname}/gen-${gen}/${basename}_${output}.h
                ${CMAKE_BINARY_DIR}/gen/${dirname}/gen-${gen}/${basename}_${output}.cpp)
        endforeach(output)
    endforeach(gen)
    if (${arg_OPTIONS})
        set(THRIFTOPT ${arg_OPTIONS})
    else()
        set(THRIFTOPT ${THRIFTC2OPTIONS})
    endif()
    add_custom_command(
        OUTPUT
            ${outputs}
            ${services}
        COMMAND
            export PYTHONPATH=${CMAKE_SOURCE_DIR}/external/fbthrift/thrift/.python-local/lib/python/\;
            mkdir -p ${CMAKE_BINARY_DIR}/gen/${dirname}\;
            ${THRIFTC} -o ${CMAKE_BINARY_DIR}/gen/${dirname} ${CMAKE_SOURCE_DIR}/${arg_THRIFTSRC}\;
            ${THRIFTC2}${arg_OPTIONS} -o ${CMAKE_BINARY_DIR}/gen/${dirname} ${CMAKE_SOURCE_DIR}/${arg_THRIFTSRC};
    )

endfunction(fboss_add_thrift)

fboss_add_thrift(THRIFTSRC common/fb303/if/fb303.thrift SERVICES FacebookService)
fboss_add_thrift(THRIFTSRC common/network/if/Address.thrift
    OPTIONS json,compatibility,include_prefix=common/network/if
    REFLECT Address)
fboss_add_thrift(THRIFTSRC fboss/agent/switch_config.thrift
    OPTIONS json
    REFLECT switch_config)
fboss_add_thrift(THRIFTSRC fboss/agent/hw/sim/sim_ctrl.thrift SERVICES SimCtrl)
fboss_add_thrift(THRIFTSRC fboss/agent/if/ctrl.thrift
    SERVICES FbossCtrl NeighborListenerClient)
fboss_add_thrift(THRIFTSRC fboss/agent/if/fboss.thrift)
fboss_add_thrift(THRIFTSRC fboss/agent/if/highres.thrift SERVICES FbossHighresClient)
fboss_add_thrift(THRIFTSRC fboss/agent/if/optic.thrift)
fboss_add_thrift(THRIFTSRC fboss/agent/if/sflow.thrift)
fboss_add_thrift(THRIFTSRC fboss/agent/hw/bcm/packettrace.thrift)
fboss_add_thrift(THRIFTSRC fboss/agent/hw/bcm/bcmswitch.thrift)
fboss_add_thrift(THRIFTSRC fboss/agent/hw/bcm/hardware_stats.thrift)
fboss_add_thrift(THRIFTSRC fboss/pcap_distribution_service/if/pcap_pubsub.thrift
    SERVICES PcapSubscriber PcapPushSubscriber)
fboss_add_thrift(THRIFTSRC fboss/qsfp_service/if/transceiver.thrift)
fboss_add_thrift(THRIFTSRC fboss/qsfp_service/if/qsfp.thrift
    SERVICES QsfpService)
