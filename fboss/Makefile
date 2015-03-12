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

INCLUDE=. .. agent/if/gen-cpp2 common/fb303/if/gen-cpp2 \
  common/network/if/gen-cpp2
ISYSTEM=$(OPENNSL_INCLUDE) \
	$(IPROUTE2_INCLUDE) \
	$(EXTRA_INCLUDE_DIRS)
LDFLAGS=$(addprefix -L,$(EXTRA_LIB_DIRS))

CXXFLAGS+=-std=gnu++11 $(addprefix -I,$(INCLUDE))\
 $(addprefix -isystem,$(ISYSTEM)) -DINCLUDE_L3 -DLONGS_ARE_64BITS

THRIFT_CPP2_OPTS = json
THRIFTC=$(THRIFT1) -I .. -I . -gen cpp:json,templates
THRIFTC2=python -mthrift_compiler.main -I .. -I . \
	 --gen cpp2:$(THRIFT_CPP2_OPTS)

OBJS=\
 agent/ApplyThriftConfig.o\
 agent/ArpHandler.o\
 agent/DHCPv4Handler.o\
 agent/DHCPv6Handler.o\
 agent/HwSwitch.o\
 agent/IPv4Handler.o\
 agent/IPv6Handler.o\
 agent/IPHeaderV4.o\
 agent/LldpManager.o\
 agent/Main.o\
 agent/NeighborUpdater.o\
 agent/Platform.o\
 agent/PortStats.o\
 agent/SfpMap.o\
 agent/SfpModule.o\
 agent/SwSwitch.o\
 agent/SwitchStats.o\
 agent/ThriftHandler.o\
 agent/TunIntf.o\
 agent/TunManager.o\
 agent/UDPHeader.o\
 agent/Utils.o\
 agent/capture/PcapFile.o\
 agent/capture/PcapPkt.o\
 agent/capture/PcapQueue.o\
 agent/capture/PcapWriter.o\
 agent/capture/PktCapture.o\
 agent/capture/PktCaptureManager.o\
 agent/gen-cpp/switch_config_reflection.o\
 agent/gen-cpp/switch_config_types.o\
 agent/hw/bcm/BcmAPI.o\
 agent/hw/bcm/BcmEgress.o\
 agent/hw/bcm/BcmHost.o\
 agent/hw/bcm/BcmIntf.o\
 agent/hw/bcm/BcmPort.o\
 agent/hw/bcm/BcmPortTable.o\
 agent/hw/bcm/BcmRoute.o\
 agent/hw/bcm/BcmRxPacket.o\
 agent/hw/bcm/BcmStats.o\
 agent/hw/bcm/BcmSwitch.o\
 agent/hw/bcm/BcmSwitchEvent.o\
 agent/hw/bcm/BcmSwitchEventCallback.o\
 agent/hw/bcm/BcmSwitchEventManager.o\
 agent/hw/bcm/BcmTxPacket.o\
 agent/hw/bcm/BcmWarmBootCache.o\
 agent/hw/mock/MockRxPacket.o\
 agent/hw/mock/MockTxPacket.o\
 agent/hw/sim/SimHandler.o\
 agent/hw/sim/SimPlatform.o\
 agent/hw/sim/SimSwitch.o\
 agent/hw/sim/gen-cpp2/SimCtrl.o\
 agent/hw/sim/gen-cpp2/sim_ctrl_types.o\
 agent/if/gen-cpp2/FbossCtrl.o\
 agent/if/gen-cpp2/ctrl_types.o\
 agent/if/gen-cpp2/fboss_types.o\
 agent/if/gen-cpp2/optic_types.o\
 agent/ndp/IPv6RouteAdvertiser.o\
 agent/oss/ApplyThriftConfig.o\
 agent/oss/Main.o\
 agent/oss/SwSwitch.o\
 agent/oss/hw/bcm/BcmAPI.o\
 agent/oss/hw/bcm/BcmSwitch.o\
 agent/oss/hw/bcm/BcmUnit.o\
 agent/oss/platforms/wedge/WedgePlatform.o\
 agent/packet/ArpHdr.o\
 agent/state/ArpTable.o\
 agent/packet/DHCPv4Packet.o\
 agent/packet/DHCPv6Packet.o\
 agent/packet/EthHdr.o\
 agent/packet/ICMPHdr.o\
 agent/packet/IPv4Hdr.o\
 agent/packet/IPv6Hdr.o\
 agent/packet/LlcHdr.o\
 agent/packet/NDPRouterAdvertisement.o\
 agent/packet/PktUtil.o\
 agent/state/ArpEntry.o\
 agent/state/ArpResponseTable.o\
 agent/state/Interface.o\
 agent/state/InterfaceMap.o\
 agent/state/NdpEntry.o\
 agent/state/NdpResponseTable.o\
 agent/state/NdpTable.o\
 agent/state/NeighborResponseTable.o\
 agent/state/NodeBase.o\
 agent/state/Port.o\
 agent/state/PortMap.o\
 agent/state/Route.o\
 agent/state/RouteDelta.o\
 agent/state/RouteForwardInfo.o\
 agent/state/RouteTable.o\
 agent/state/RouteTableMap.o\
 agent/state/RouteTableRib.o\
 agent/state/RouteTypes.o\
 agent/state/RouteUpdater.o\
 agent/state/StateDelta.o\
 agent/state/SwitchState.o\
 agent/state/Vlan.o\
 agent/state/VlanMap.o\
 agent/state/VlanMapDelta.o\
 common/fb303/if/gen-cpp2/FacebookService.o\
 common/fb303/if/gen-cpp2/fb303_types.o\
 common/network/if/gen-cpp/Address_reflection.o\
 common/network/if/gen-cpp/Address_types.o\
 common/network/if/gen-cpp2/Address_types.o\
 common/stats/ServiceData.o

WEDGE_OBJS=agent/platforms/wedge/WedgePlatform.o\
 agent/platforms/wedge/WedgePort.o\
 agent/platforms/wedge/wedge_ctrl.o

SIM_OBJS=agent/platforms/sim/sim_ctrl.o

THRIFT = \
  agent/hw/sim/sim_ctrl.gen-cpp2 \
  agent/if/ctrl.gen-cpp2 \
  agent/if/fboss.gen-cpp2 \
  agent/if/optic.gen-cpp2 \
  agent/switch_config.gen-cpp \
  common/fb303/if/fb303.gen-cpp2 \
  common/network/if/Address.gen-cpp \
  common/network/if/Address.gen-cpp2
THRIFT_MADE_FILES = $(patsubst %,%.MADE,$(THRIFT))

LIBS=boost_filesystem boost_system double-conversion folly gflags glog\
 pthread thrift thriftcpp2

# Rules

all : thrift
	@$(MAKE) --no-print-directory wedge_agent sim_agent

clean :
	rm -rf sim_agent wedge_agent libfboss_agent.a\
	  $(join $(dir $(THRIFT)),$(subst .,,$(suffix $(THRIFT))))\
	  $(OBJS) $(WEDGE_OBJS) $(SIM_OBJS) $(THRIFT_MADE_FILES)

thrift : $(THRIFT_MADE_FILES)

wedge_agent : libfboss_agent.a $(WEDGE_OBJS)
	g++ -o $@ $(WEDGE_OBJS)\
	 $(LDFLAGS)\
	 $(addprefix -Xlinker ,$< $(IPROUTE2_LIB) $(OPENNSL_LIB))\
	 $(addprefix -l,$(LIBS))

sim_agent : libfboss_agent.a $(SIM_OBJS)
	g++ -o $@ $(SIM_OBJS)\
	 $(LDFLAGS)\
	 $(addprefix -Xlinker ,$< $(IPROUTE2_LIB) $(OPENNSL_LIB))\
	 $(addprefix -l,$(LIBS))

libfboss_agent.a : $(OBJS)
	ar rcs $@ $(OBJS)
	touch $@

# Address is generated with special options
common/network/if/Address.gen-cpp2.MADE: \
    THRIFT_CPP2_OPTS=json,compatibility,include_prefix=common/network/if

.PHONY : all clean thrift

# Pattern rules

%.gen-cpp.MADE: %.thrift
	$(THRIFTC) -o $(@D) $^ 
	@touch $@

%.gen-cpp2.MADE: %.thrift
	$(THRIFTC2) -o $(@D) $^
	@touch $@
