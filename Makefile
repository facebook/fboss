#
# Makefile for building FBOSS.
# We're planning on replacing this with a CMake-based build system soon,
# but for now this should at least help perform an initial build.
#
# Settings that likely will need to be modified for your environment:
#
OPENNSL = /usr/local/src/OpenNSL
IPROUTE2_INCLUDE = /usr/local/src/iproute2-3.19.0/include
IPROUTE2_LIB=/usr/local/src/iproute2-3.19.0/lib/libnetlink.a
THRIFT1 = /usr/local/facebook/bin/thrift1

# Optional extra directories.  For instance, if you have folly or fbthrift
# installed in non-standard locations.
EXTRA_INCLUDE_DIRS = /usr/local/facebook/include
EXTRA_LIB_DIRS = /usr/local/facebook/lib

# Ensure OpenNSL is set and accessible
ifeq (,$(OPENNSL))
  ifeq (,$(OPENNSL_INCLUDE))
    $(error You must define OPENNSL or OPENNSL_INCLUDE and OPENNSL_LIB)
  else
    ifeq (,$(OPENNSL_LIB))
      $(error You must define OPENNSL or OPENNSL_INCLUDE and OPENNSL_LIB)
    endif
  endif
else
  OPENNSL_INCLUDE=$(OPENNSL)/include
  OPENNSL_LIB=$(OPENNSL)/bin/wedge-trident/libopennsl.so.1
endif
ifeq (,$(wildcard $(OPENNSL_LIB)))
  $(error '$(OPENNSL_LIB)' not found)
endif

# Ensure iproute2 is set and accessible
ifeq (,$(IPROUTE2_LIB))
  $(error You must define IPROUTE2_LIB)
endif
ifeq (,$(wildcard $(IPROUTE2_LIB)))
  $(error '$(IPROUTE2_LIB)' not found)
endif

INCLUDE=. build \
	build/fboss/agent/if/gen-cpp2 \
	build/common/fb303/if/gen-cpp2 \
	build/common/network/if/gen-cpp2
ISYSTEM=$(OPENNSL_INCLUDE) \
	$(IPROUTE2_INCLUDE) \
	$(EXTRA_INCLUDE_DIRS)
LDFLAGS=$(addprefix -L,$(EXTRA_LIB_DIRS))

WARNINGS = -Wall -Werror \
	   -Wno-sign-compare \
	   -Wno-unused-variable \
	   -Woverloaded-virtual \
	   -Wnon-virtual-dtor \
	   -Wno-maybe-uninitialized \
	   -Wdeprecated-declarations \
	   -Wno-error=deprecated-declarations

CXXFLAGS += -std=gnu++11 -g $(WARNINGS) \
	    $(addprefix -I,$(INCLUDE)) \
	    $(addprefix -isystem,$(ISYSTEM)) \
	    -DINCLUDE_L3 -DLONGS_ARE_64BITS

THRIFT_CPP2_OPTS = json
THRIFTC=$(THRIFT1) -I . -gen cpp:json,templates
THRIFTC2=python -mthrift_compiler.main -I . \
	 --gen cpp2:$(THRIFT_CPP2_OPTS)

SRCS=\
 fboss/agent/ApplyThriftConfig.cpp \
 fboss/agent/ArpHandler.cpp \
 fboss/agent/DHCPv4Handler.cpp \
 fboss/agent/DHCPv6Handler.cpp \
 fboss/agent/HwSwitch.cpp \
 fboss/agent/IPv4Handler.cpp \
 fboss/agent/IPv6Handler.cpp \
 fboss/agent/IPHeaderV4.cpp \
 fboss/agent/LldpManager.cpp \
 fboss/agent/Main.cpp \
 fboss/agent/NeighborUpdater.cpp \
 fboss/agent/Platform.cpp \
 fboss/agent/PortStats.cpp \
 fboss/agent/SfpMap.cpp \
 fboss/agent/SfpModule.cpp \
 fboss/agent/SwSwitch.cpp \
 fboss/agent/SwitchStats.cpp \
 fboss/agent/ThriftHandler.cpp \
 fboss/agent/TunIntf.cpp \
 fboss/agent/TunManager.cpp \
 fboss/agent/UDPHeader.cpp \
 fboss/agent/Utils.cpp \
 fboss/agent/capture/PcapFile.cpp \
 fboss/agent/capture/PcapPkt.cpp \
 fboss/agent/capture/PcapQueue.cpp \
 fboss/agent/capture/PcapWriter.cpp \
 fboss/agent/capture/PktCapture.cpp \
 fboss/agent/capture/PktCaptureManager.cpp \
 fboss/agent/gen-cpp/switch_config_reflection.cpp \
 fboss/agent/gen-cpp/switch_config_types.cpp \
 fboss/agent/hw/bcm/BcmAPI.cpp \
 fboss/agent/hw/bcm/BcmEgress.cpp \
 fboss/agent/hw/bcm/BcmHost.cpp \
 fboss/agent/hw/bcm/BcmIntf.cpp \
 fboss/agent/hw/bcm/BcmPort.cpp \
 fboss/agent/hw/bcm/BcmPortTable.cpp \
 fboss/agent/hw/bcm/BcmRoute.cpp \
 fboss/agent/hw/bcm/BcmRxPacket.cpp \
 fboss/agent/hw/bcm/BcmStats.cpp \
 fboss/agent/hw/bcm/BcmSwitch.cpp \
 fboss/agent/hw/bcm/BcmSwitchEvent.cpp \
 fboss/agent/hw/bcm/BcmSwitchEventCallback.cpp \
 fboss/agent/hw/bcm/BcmSwitchEventManager.cpp \
 fboss/agent/hw/bcm/BcmTxPacket.cpp \
 fboss/agent/hw/bcm/BcmWarmBootCache.cpp \
 fboss/agent/hw/mock/MockRxPacket.cpp \
 fboss/agent/hw/mock/MockTxPacket.cpp \
 fboss/agent/hw/sim/SimHandler.cpp \
 fboss/agent/hw/sim/SimPlatform.cpp \
 fboss/agent/hw/sim/SimSwitch.cpp \
 fboss/agent/hw/sim/gen-cpp2/SimCtrl.cpp \
 fboss/agent/hw/sim/gen-cpp2/sim_ctrl_types.cpp \
 fboss/agent/if/gen-cpp2/FbossCtrl.cpp \
 fboss/agent/if/gen-cpp2/ctrl_types.cpp \
 fboss/agent/if/gen-cpp2/fboss_types.cpp \
 fboss/agent/if/gen-cpp2/optic_types.cpp \
 fboss/agent/ndp/IPv6RouteAdvertiser.cpp \
 fboss/agent/oss/ApplyThriftConfig.cpp \
 fboss/agent/oss/Main.cpp \
 fboss/agent/oss/SwSwitch.cpp \
 fboss/agent/oss/hw/bcm/BcmAPI.cpp \
 fboss/agent/oss/hw/bcm/BcmSwitch.cpp \
 fboss/agent/oss/hw/bcm/BcmUnit.cpp \
 fboss/agent/oss/platforms/wedge/WedgePlatform.cpp \
 fboss/agent/packet/ArpHdr.cpp \
 fboss/agent/state/ArpTable.cpp \
 fboss/agent/packet/DHCPv4Packet.cpp \
 fboss/agent/packet/DHCPv6Packet.cpp \
 fboss/agent/packet/EthHdr.cpp \
 fboss/agent/packet/ICMPHdr.cpp \
 fboss/agent/packet/IPv4Hdr.cpp \
 fboss/agent/packet/IPv6Hdr.cpp \
 fboss/agent/packet/LlcHdr.cpp \
 fboss/agent/packet/NDPRouterAdvertisement.cpp \
 fboss/agent/packet/PktUtil.cpp \
 fboss/agent/state/ArpEntry.cpp \
 fboss/agent/state/ArpResponseTable.cpp \
 fboss/agent/state/Interface.cpp \
 fboss/agent/state/InterfaceMap.cpp \
 fboss/agent/state/NdpEntry.cpp \
 fboss/agent/state/NdpResponseTable.cpp \
 fboss/agent/state/NdpTable.cpp \
 fboss/agent/state/NeighborResponseTable.cpp \
 fboss/agent/state/NodeBase.cpp \
 fboss/agent/state/Port.cpp \
 fboss/agent/state/PortMap.cpp \
 fboss/agent/state/Route.cpp \
 fboss/agent/state/RouteDelta.cpp \
 fboss/agent/state/RouteForwardInfo.cpp \
 fboss/agent/state/RouteTable.cpp \
 fboss/agent/state/RouteTableMap.cpp \
 fboss/agent/state/RouteTableRib.cpp \
 fboss/agent/state/RouteTypes.cpp \
 fboss/agent/state/RouteUpdater.cpp \
 fboss/agent/state/StateDelta.cpp \
 fboss/agent/state/SwitchState.cpp \
 fboss/agent/state/Vlan.cpp \
 fboss/agent/state/VlanMap.cpp \
 fboss/agent/state/VlanMapDelta.cpp \
 common/fb303/if/gen-cpp2/FacebookService.cpp \
 common/fb303/if/gen-cpp2/fb303_types.cpp \
 common/network/if/gen-cpp/Address_reflection.cpp \
 common/network/if/gen-cpp/Address_types.cpp \
 common/network/if/gen-cpp2/Address_types.cpp \
 common/stats/ServiceData.cpp
OBJS = $(patsubst %.cpp,build/%.o,$(SRCS))

WEDGE_SRCS = \
 fboss/agent/platforms/wedge/WedgePlatform.cpp \
 fboss/agent/platforms/wedge/WedgePort.cpp \
 fboss/agent/platforms/wedge/wedge_ctrl.cpp
WEDGE_OBJS = $(patsubst %.cpp,build/%.o,$(WEDGE_SRCS))

SIM_SRCS = fboss/agent/platforms/sim/sim_ctrl.cpp
SIM_OBJS = $(patsubst %.cpp,build/%.o,$(SIM_SRCS))

THRIFT = \
  fboss/agent/hw/sim/sim_ctrl.gen-cpp2 \
  fboss/agent/if/ctrl.gen-cpp2 \
  fboss/agent/if/fboss.gen-cpp2 \
  fboss/agent/if/optic.gen-cpp2 \
  fboss/agent/switch_config.gen-cpp \
  common/fb303/if/fb303.gen-cpp2 \
  common/network/if/Address.gen-cpp \
  common/network/if/Address.gen-cpp2
THRIFT_MADE_FILES = $(patsubst %,build/%.MADE,$(THRIFT))

LIBS=boost_filesystem boost_system double-conversion folly gflags glog\
 pthread thrift thriftcpp2

# Rules

all : thrift
	@$(MAKE) --no-print-directory wedge_agent sim_agent

clean :
	$(RM) sim_agent wedge_agent libfboss_agent.a
	$(RM) -r build

thrift : $(THRIFT_MADE_FILES)

wedge_agent : build/libfboss_agent.a $(WEDGE_OBJS)
	g++ -o $@ $(WEDGE_OBJS)\
	 $(LDFLAGS)\
	 $(addprefix -Xlinker ,$< $(IPROUTE2_LIB) $(OPENNSL_LIB))\
	 $(addprefix -l,$(LIBS))

sim_agent : build/libfboss_agent.a $(SIM_OBJS)
	g++ -o $@ $(SIM_OBJS)\
	 $(LDFLAGS)\
	 $(addprefix -Xlinker ,$< $(IPROUTE2_LIB) $(OPENNSL_LIB))\
	 $(addprefix -l,$(LIBS))

build/libfboss_agent.a : $(OBJS)
	ar rcs $@ $(OBJS)
	touch $@

# Address is generated with special options
build/common/network/if/Address.gen-cpp2.MADE: \
    THRIFT_CPP2_OPTS=json,compatibility,include_prefix=common/network/if

.PHONY : all clean thrift

# Pattern rules

build/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $@ $<

build/%.o: build/%.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $@ $<

build/%.gen-cpp.MADE: %.thrift
	@mkdir -p $(@D)
	$(THRIFTC) -o $(@D) $^ 
	@touch $@

build/%.gen-cpp2.MADE: %.thrift
	@mkdir -p $(@D)
	$(THRIFTC2) -o $(@D) $^
	@touch $@
