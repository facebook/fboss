cmake_minimum_required(VERSION 2.8)
project(FBOSS)
include(CMakeParseArguments)

# Ideally, we would use
#    set_property(TARGET fboss_agent PROPERTY CXX_STANDARD 11)
# to define the C++ version, but we don't want to depend on
# cmake 3.1, as it's not currently widely available. We only support
# building on Linux under GCC right now, so we might as well just hard
# code the flags.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -Wno-sign-compare")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable -Woverloaded-virtual -Wnon-virtual-dtor")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-maybe-uninitialized -Wdeprecated-declarations")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=deprecated-declarations")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DINCLUDE_L3 -DLONGS_ARE_64BITS")
set(CMAKE_PROGRAM_PATH "${CMAKE_SOURCE_DIR}/external/fbthrift/thrift/compiler/;${CMAKE_PROGRAM_PATH}")

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR}/gen)
include_directories(${CMAKE_BINARY_DIR}/gen/fboss/agent/if/gen-cpp2)
include_directories(${CMAKE_BINARY_DIR}/gen/common/fb303/if/gen-cpp2)
include_directories(${CMAKE_BINARY_DIR}/gen/common/network/if/gen-cpp2)

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

# External libraries that are not generally available. Look in external/
# for these. This is where getdeps.sh will toss them.
find_library(FOLLY folly PATHS ${CMAKE_SOURCE_DIR}/external/folly/folly/.libs)
find_library(THRIFT thrift PATHS ${CMAKE_SOURCE_DIR}/external/fbthrift/thrift/lib/cpp/.libs)
find_library(THRIFTCPP2 thriftcpp2 PATHS ${CMAKE_SOURCE_DIR}/external/fbthrift/thrift/lib/cpp2/.libs)
find_library(OPENNSL opennsl PATHS ${CMAKE_SOURCE_DIR}/external/OpenNSL/bin/wedge-trident/)
find_library(IPROUTE2 netlink PATHS ${CMAKE_SOURCE_DIR}/external/iproute2/lib/)

include_directories(${CMAKE_SOURCE_DIR}/external/OpenNSL/include)
include_directories(${CMAKE_SOURCE_DIR}/external/iproute2/include)
include_directories(${CMAKE_SOURCE_DIR}/external/folly)
include_directories(${CMAKE_SOURCE_DIR}/external/fbthrift)

add_executable(wedge_agent
    fboss/agent/platforms/wedge/WedgePlatform.cpp
    fboss/agent/platforms/wedge/WedgeProductInfo.cpp
    fboss/agent/platforms/wedge/WedgePort.cpp
    fboss/agent/platforms/wedge/wedge_ctrl.cpp
    fboss/agent/platforms/wedge/oss/WedgePort.cpp
    fboss/agent/platforms/wedge/oss/WedgePlatform.cpp
)
target_link_libraries(wedge_agent fboss_agent)

add_executable(sim_agent
    fboss/agent/platforms/sim/sim_ctrl.cpp
)
target_link_libraries(sim_agent fboss_agent)

add_library(fboss_agent STATIC
    common/stats/ServiceData.cpp
    fboss/agent/ApplyThriftConfig.cpp
    fboss/agent/ArpHandler.cpp
    fboss/agent/DHCPv4Handler.cpp
    fboss/agent/DHCPv6Handler.cpp
    fboss/agent/HighresCounterSubscriptionHandler.cpp
    fboss/agent/HighresCounterUtil.cpp
    fboss/agent/HwSwitch.cpp
    fboss/agent/I2c.cpp
    fboss/agent/IPHeaderV4.cpp
    fboss/agent/IPv4Handler.cpp
    fboss/agent/IPv6Handler.cpp
    fboss/agent/LldpManager.cpp
    fboss/agent/Main.cpp
    fboss/agent/NeighborListenerClient.cpp
    fboss/agent/NeighborUpdater.cpp
    fboss/agent/Platform.cpp
    fboss/agent/PortStats.cpp
    fboss/agent/RestClient.cpp
    fboss/agent/SfpModule.cpp
    fboss/agent/SwitchStats.cpp
    fboss/agent/SwSwitch.cpp
    fboss/agent/ThriftHandler.cpp
    fboss/agent/TransceiverMap.cpp
    fboss/agent/TunIntf.cpp
    fboss/agent/TunManager.cpp
    fboss/agent/UDPHeader.cpp
    fboss/agent/Utils.cpp
    fboss/agent/capture/PcapFile.cpp
    fboss/agent/capture/PcapPkt.cpp
    fboss/agent/capture/PcapQueue.cpp
    fboss/agent/capture/PcapWriter.cpp
    fboss/agent/capture/PktCapture.cpp
    fboss/agent/capture/PktCaptureManager.cpp
    fboss/agent/hw/bcm/BcmAPI.cpp
    fboss/agent/hw/bcm/BcmEgress.cpp
    fboss/agent/hw/bcm/BcmHost.cpp
    fboss/agent/hw/bcm/BcmIntf.cpp
    fboss/agent/hw/bcm/BcmPort.cpp
    fboss/agent/hw/bcm/BcmPortTable.cpp
    fboss/agent/hw/bcm/BcmRoute.cpp
    fboss/agent/hw/bcm/BcmRxPacket.cpp
    fboss/agent/hw/bcm/BcmPlatform.cpp
    fboss/agent/hw/bcm/BcmStats.cpp
    fboss/agent/hw/bcm/BcmSwitch.cpp
    fboss/agent/hw/bcm/BcmSwitchEvent.cpp
    fboss/agent/hw/bcm/BcmSwitchEventCallback.cpp
    fboss/agent/hw/bcm/BcmSwitchEventManager.cpp
    fboss/agent/hw/bcm/BcmTxPacket.cpp
    fboss/agent/hw/bcm/BcmWarmBootCache.cpp
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
    fboss/agent/oss/SwSwitch.cpp
    fboss/agent/hw/bcm/oss/BcmAPI.cpp
    fboss/agent/hw/bcm/oss/BcmHost.cpp
    fboss/agent/hw/bcm/oss/BcmPort.cpp
    fboss/agent/hw/bcm/oss/BcmSwitch.cpp
    fboss/agent/hw/bcm/oss/BcmUnit.cpp
    fboss/agent/hw/bcm/oss/BcmWarmBootHelper.cpp
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
    fboss/agent/state/ArpEntry.cpp
    fboss/agent/state/ArpResponseTable.cpp
    fboss/agent/state/ArpTable.cpp
    fboss/agent/state/Interface.cpp
    fboss/agent/state/InterfaceMap.cpp
    fboss/agent/state/NdpEntry.cpp
    fboss/agent/state/NdpResponseTable.cpp
    fboss/agent/state/NdpTable.cpp
    fboss/agent/state/NeighborResponseTable.cpp
    fboss/agent/state/NodeBase.cpp
    fboss/agent/state/Port.cpp
    fboss/agent/state/PortMap.cpp
    fboss/agent/state/Route.cpp
    fboss/agent/state/RouteDelta.cpp
    fboss/agent/state/RouteForwardInfo.cpp
    fboss/agent/state/RouteTable.cpp
    fboss/agent/state/RouteTableMap.cpp
    fboss/agent/state/RouteTableRib.cpp
    fboss/agent/state/RouteTypes.cpp
    fboss/agent/state/RouteUpdater.cpp
    fboss/agent/state/StateDelta.cpp
    fboss/agent/state/SwitchState.cpp
    fboss/agent/state/Vlan.cpp
    fboss/agent/state/VlanMap.cpp
    fboss/agent/state/VlanMapDelta.cpp
    # generated sources
    ${CMAKE_BINARY_DIR}/gen/common/fb303/if/gen-cpp2/FacebookService.cpp
    ${CMAKE_BINARY_DIR}/gen/common/fb303/if/gen-cpp2/FacebookService_client.cpp
    ${CMAKE_BINARY_DIR}/gen/common/fb303/if/gen-cpp2/fb303_types.cpp
    ${CMAKE_BINARY_DIR}/gen/common/network/if/gen-cpp2/Address_types.cpp
    ${CMAKE_BINARY_DIR}/gen/common/network/if/gen-cpp/Address_types.cpp
    ${CMAKE_BINARY_DIR}/gen/common/network/if/gen-cpp/Address_reflection.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/gen-cpp/switch_config_reflection.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/gen-cpp/switch_config_types.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/hw/sim/gen-cpp2/SimCtrl.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/hw/sim/gen-cpp2/sim_ctrl_types.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/if/gen-cpp2/ctrl_types.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/if/gen-cpp2/FbossCtrl.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/if/gen-cpp2/NeighborListenerClient_client.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/if/gen-cpp2/FbossHighresClient_client.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/if/gen-cpp2/fboss_types.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/if/gen-cpp2/optic_types.cpp
    ${CMAKE_BINARY_DIR}/gen/fboss/agent/if/gen-cpp2/highres_types.cpp
)
target_link_libraries(fboss_agent
    ${GLOG}
    ${FOLLY}
    ${GFLAGS}
    ${GLOG}
    ${PTHREAD}
    ${THRIFT}
    ${THRIFTCPP2}
    ${OPENNSL}
    ${IPROUTE2}
    ${BOOSTFS}
    ${BOOSTSER}
    ${BOOSTSYS}
    ${BOOSTTHREAD}
    ${DOUBLECONV}
)

find_program(THRIFT1 thrift1)
find_program(PYTHON python)
set(THRIFTC2OPTS json)
set(THRIFTC ${THRIFT1} -I ${CMAKE_SOURCE_DIR} -gen cpp:json,templates)
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
    foreach(gen cpp cpp2)
        foreach(reflect ${arg_REFLECT})
            set(outputs ${outputs} 
                ${CMAKE_BINARY_DIR}/gen/${dirname}/gen-${gen}/${reflect}_reflection.h
                ${CMAKE_BINARY_DIR}/gen/${dirname}/gen-${gen}/${reflect}_reflection.cpp)
        endforeach(reflect)
        foreach(output constants types)
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
    REFLECT switch_config)
fboss_add_thrift(THRIFTSRC fboss/agent/hw/sim/sim_ctrl.thrift SERVICES SimCtrl)
fboss_add_thrift(THRIFTSRC fboss/agent/if/ctrl.thrift 
    SERVICES FbossCtrl NeighborListenerClient)
fboss_add_thrift(THRIFTSRC fboss/agent/if/fboss.thrift)
fboss_add_thrift(THRIFTSRC fboss/agent/if/optic.thrift)
fboss_add_thrift(THRIFTSRC fboss/agent/if/highres.thrift SERVICES FbossHighresClient)

