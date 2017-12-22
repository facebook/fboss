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
set(PCAP_DIR_PATH
  "${CMAKE_BINARY_DIR}/gen/fboss/pcap_distribution_service/if/gen-cpp2/")

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_BUILD_DIR})


# Generate thrift deps and libraries
find_program(THRIFT1 thrift1 HINT
             ${CMAKE_SOURCE_DIR}/external/fbthrift/build/bin)
set(THRIFT_INCLUDE ${CMAKE_SOURCE_DIR}/external/fbthrift/thrift)
set(THRIFT_TEMPLATES ${THRIFT_INCLUDE}/compiler/generate/templates)
include(${CMAKE_SOURCE_DIR}/ThriftLibrary.cmake)

set(THRIFT_DIR ${CMAKE_SOURCE_DIR}/build/thrift-out)

#### HACK!
# Current thrift_library() macro semantics don't transitively
# infer dependencies, e.g., qsfp.thrift can't complile because
# it depends files generated from on fb303.thrift.
# So, with each call of our macro my_thrift_library()
# we accumulate the sources files and pass them as dependencies
# to the subsequent calls. Thus, the order we declare the thrift_library()
# calls informs the dependency graph.

# Create our own wrapper around the standard thrift library generator
set(THRIFT_LANGS cpp2 )
foreach(lang ${THRIFT_LANGS})
    set(THRIFT_CODE_${lang} "")
    set(THRIFT_LIBS_${lang} "") # create an empty list per lang
endforeach(lang)
macro(my_thrift_library file_name services language
                        options file_path output_path)
    file(MAKE_DIRECTORY ${output_path})
    thrift_library(
        "${file_name}"
        "${services}"
        "${language}"
        "${options}"
        "${file_path}"
        "${output_path}"
        "${THRIFT_CODE_${lang}}")
    list(APPEND THRIFT_CODE_${lang} "${${file_name}-${language}-SOURCES}")
    list(APPEND THRIFT_LIBS_${language} "${file_name}-${language}")
endmacro(my_thrift_library)

# Now generate code for each .thrift file
foreach(lang ${THRIFT_LANGS})
    my_thrift_library(
        "fb303" #file_name
        "FacebookService" #services
        "${lang}" #language
        "" #options
        "${CMAKE_SOURCE_DIR}/common/fb303/if" #file_path
        "${THRIFT_DIR}/common/fb303/if" #output_path
        )
# fboss_add_thrift(THRIFTSRC common/network/if/Address.thrift
#     OPTIONS json,compatibility,include_prefix=common/network/if
#     REFLECT Address)
    my_thrift_library(
        "Address" #file_name
        "" #services
        "${lang}" #language
        #"json,compatibility,include_prefix=common/network/if" #options
        "include_prefix=common/network/if" # options
        "${CMAKE_SOURCE_DIR}/common/network/if" #file_path
        "${THRIFT_DIR}/common/network/if" #output_path
        ""  # deps
        )
# fboss_add_thrift(THRIFTSRC fboss/agent/switch_config.thrift
#     OPTIONS json
#     REFLECT switch_config)
    my_thrift_library(
        "highres" #file_name
        "FbossHighresClient" #services
        "${lang}" #language
        "" #options
        "${CMAKE_SOURCE_DIR}/fboss/agent/if" #file_path
        "${THRIFT_DIR}/fboss/agent/if" #output_path
        )
    my_thrift_library(
        "switch_config" #file_name
        "" #services
        "${lang}" #language
        "json" #options
        "${CMAKE_SOURCE_DIR}/fboss/agent" #file_path
        "${THRIFT_DIR}/fboss/agent" #output_path
        ""  # deps
        )
    my_thrift_library(
        "transceiver" #file_name
        "" #services
        "${lang}" #language
        "" #options
        "${CMAKE_SOURCE_DIR}/fboss/qsfp_service/if" #file_path
        "${THRIFT_DIR}/fboss/qsfp_service/if" #output_path
        )
    my_thrift_library(
        "optic" #file_name
        "" #services
        "${lang}" #language
        "" #options
        "${CMAKE_SOURCE_DIR}/fboss/agent/if" #file_path
        "${THRIFT_DIR}/fboss/agent/if" #output_path
        )
    foreach(lib fboss sflow)
        my_thrift_library(
            "${lib}" #file_name
            "" #services
            "${lang}" #language
            "" #options
            "${CMAKE_SOURCE_DIR}/fboss/agent/if" #file_path
            "${THRIFT_DIR}/fboss/agent/if" #output_path
            )
    endforeach(lib)
    my_thrift_library(
        "ctrl" #file_name
        "NeighborListenerClient;FbossCtrl" #services
        "${lang}" #language
        "FbossHighresClient" #options
        "${CMAKE_SOURCE_DIR}/fboss/agent/if" #file_path
        "${THRIFT_DIR}/fboss/agent/if" #output_path
        )
    my_thrift_library(
        "sim_ctrl" #file_name
        "SimCtrl" #services
        "${lang}" #language
        "" #options
        "${CMAKE_SOURCE_DIR}/fboss/agent/hw/sim" #file_path
        "${THRIFT_DIR}/fboss/agent/hw/sim" #output_path
        )
    foreach(lib packettrace bcmswitch hardware_stats)
        my_thrift_library(
            "${lib}" #file_name
            "" #services
            "${lang}" #language
            "" #options
            "${CMAKE_SOURCE_DIR}/fboss/agent/hw/bcm" #file_path
            "${THRIFT_DIR}/fboss/agent/hw/bcm" #output_path
            )
    endforeach(lib)
    my_thrift_library(
        "pcap_pubsub" #file_name
        "PcapPushSubscriber" #services
        "${lang}" #language
        "" #options
        "${CMAKE_SOURCE_DIR}/fboss/pcap_distribution_service/if" #file_path
        "${THRIFT_DIR}/fboss/pcap_distribution_service/if" #output_path
        )
    my_thrift_library(
        "qsfp" #file_name
        "QsfpService" #services
        "${lang}" #language
        "" #options
        "${CMAKE_SOURCE_DIR}/fboss/qsfp_service/if" #file_path
        "${THRIFT_DIR}/fboss/qsfp_service/if" #output_path
        )
        my_thrift_library(
            "netlink_manager_service" #file_name
            "NetlinkManagerService" #services
            "${lang}" #language
            "" #options
            "${CMAKE_SOURCE_DIR}/fboss/netlink_manager/if" #file_path
            "${THRIFT_DIR}/fboss/netlink_manager/if" #output_path
        )
endforeach(lang)

include_directories(${THRIFT_DIR})
# message("Building Thrift libraries: ${THRIFT_LIBS_cpp2}")


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
find_library(GSSAPI gssapi_krb5)
find_library(DL dl)
find_library(EVENT event)
find_library(NETLINK3 nl-3)
find_library(NETLINKROUTE3 nl-route-3)
find_library(NUMA numa)
find_library(CURL curl)

# External libraries that are not generally available. Look in external/
# for these. This is where getdeps.sh will toss them.
find_library(FOLLY folly PATHS ${CMAKE_SOURCE_DIR}/external/folly/folly/.libs)
find_library(FOLLYBENCHMARK follybenchmark
             PATHS ${CMAKE_SOURCE_DIR}/external/folly/folly/.libs)
find_library(WANGLE wangle
             PATHS ${CMAKE_SOURCE_DIR}/external/wangle/wangle/build/lib)
find_library(THRIFT thrift
             PATHS ${CMAKE_SOURCE_DIR}/external/fbthrift/build/lib/)
find_library(PROTOCOL protocol
             PATHS ${CMAKE_SOURCE_DIR}/external/fbthrift/build/lib)
find_library(THRIFTPROTO thriftprotocol
             PATHS ${CMAKE_SOURCE_DIR}/external/fbthrift/build/lib)
find_library(THRIFTCPP2 thriftcpp2
             PATHS ${CMAKE_SOURCE_DIR}/external/fbthrift/build/lib)
find_library(THRIFTTRANSPORT transport
             PATHS ${CMAKE_SOURCE_DIR}/external/fbthrift/build/lib)
find_library(THRIFTASYNC async
             PATHS ${CMAKE_SOURCE_DIR}/external/fbthrift/build/lib)
find_library(THRIFTCONCURENCY concurrency
             PATHS ${CMAKE_SOURCE_DIR}/external/fbthrift/build/lib)
find_library(THRIFTCORE thrift-core
             PATHS ${CMAKE_SOURCE_DIR}/external/fbthrift/build/lib)
find_library(THRIFTFROZEN thriftfrozen2
             PATHS ${CMAKE_SOURCE_DIR}/external/fbthrift/build/lib)
find_library(OPENNSL opennsl
             PATHS ${CMAKE_SOURCE_DIR}/external/OpenNSL/bin/wedge-trident)
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
    fboss/agent/hw/bcm/BcmAclEntry.cpp
    fboss/agent/hw/bcm/BcmAclTable.cpp
    fboss/agent/hw/bcm/BcmAPI.cpp
    fboss/agent/hw/bcm/BcmEgress.cpp
    fboss/agent/hw/bcm/BcmHost.cpp
    fboss/agent/hw/bcm/BcmHostKey.cpp
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
    fboss/agent/hw/bcm/oss/BcmWarmBootCache.cpp
    fboss/agent/hw/bcm/oss/BcmWarmBootHelper.cpp
    fboss/agent/hw/bcm/oss/BcmTableStats.cpp
    fboss/agent/hw/bcm/Utils.cpp
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
    fboss/agent/state/Interface.cpp
    fboss/agent/state/InterfaceMap.cpp
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
    fboss/qsfp_service/lib/QsfpClient.cpp
    fboss/qsfp_service/lib/QsfpCache.cpp

)

set(ALL_THRIFT_LIBS
    ${THRIFT}
    ${PROTOCOL}
    ${THRIFTPROTO}
    ${THRIFTCPP2}
    ${THRIFTASYNC}
    ${THRIFTCONCURENCY}
    ${THRIFTCORE}
    ${THRIFTFROZEN}
    ${THRIFTTRANSPORT}
    )

# NOTE: target_link_library doesn't do anything for
# static (archive) files
set(LIBFBOSS_AGENT_DEPS
    "${ALL_THRIFT_LIBS}"
    "${THRIFT_LIBS_cpp2}"  # autogenerated thrift libs
    ${GLOG}
    ${SSL}
    ${PCAP}
    ${USB}
    ${ZLIB}
    ${SNAPPY}
    ${CRYPTO}
    ${GSSAPI}
    ${FOLLY}
    ${GFLAGS}
    ${GLOG}
    ${PTHREAD}
    ${WANGLE}
    "${ALL_THRIFT_LIBS}"
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
    "${THRIFT_LIBS_cpp2}"  # autogenerated thrift libs
    ${WANGLE}
)

#message("LIBFBOSS_AGENT_DEPS: ${LIBFBOSS_AGENT_DEPS}")

target_link_libraries(fboss_agent "${LIBFBOSS_AGENT_DEPS}")
target_link_libraries(wedge_agent
                        fboss_agent
                        "${LIBFBOSS_AGENT_DEPS}"
                    )
target_link_libraries(netlink_manager
                        fboss_agent
                        "${LIBFBOSS_AGENT_DEPS}"
                    )
target_link_libraries(qsfp_service
                        fboss_agent
                        "${LIBFBOSS_AGENT_DEPS}"
                     )


# TODO(rsher)
# depends on agent/hw/bcm/facebook/bcm_config.thrift
# investigate open sourcing
# add_executable(lldp_tool
#       fboss/util/lldp_tool.cpp
# )
#
# target_link_libraries(lldp_tool
#       fboss-agent
#       "${LIBFBOSS_AGENT_DEPS}"
# )

add_executable(cp2112_util
    fboss/util/cp2112_util.cpp
)
target_link_libraries(cp2112_util
       fboss_agent
       "${LIBFBOSS_AGENT_DEPS}"
)

add_executable(wedge_qsfp_util
    fboss/util/wedge_qsfp_util.cpp
)
target_link_libraries(wedge_qsfp_util
       fboss_agent
       "${LIBFBOSS_AGENT_DEPS}"
)




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
    "${LIBFBOSS_AGENT_DEPS}"
    gtest
    gmock
    ${CMAKE_THREAD_LIBS_INIT}
)
add_test(test agent_test)

#TODO: Add tests from other folders aside from agent/test
