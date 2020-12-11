// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once
#include "fboss/agent/gen-cpp2/switch_config_types.h"

namespace facebook::fboss {

class HwAsic {
 public:
  enum class Feature {
    HOSTTABLE_FOR_HOSTROUTES,
    SPAN,
    ERSPANv4,
    ERSPANv6,
    SFLOWv4,
    SFLOWv6,
    MPLS,
    MPLS_ECMP,
    TRUNCATE_MIRROR_PACKET,
    TX_VLAN_STRIPPING_ON_PORT,
    HOT_SWAP,
    HASH_FIELDS_CUSTOMIZATION,
    QUEUE,
    ECN,
    L3_QOS,
    QOS_MAP_GLOBAL,
    QCM,
    SCHEDULER_PPS,
    SMAC_EQUALS_DMAC_CHECK_ENABLED,
    NEXTHOP_TTL_DECREMENT_DISABLE,
    PORT_TTL_DECREMENT_DISABLE,
    PORT_INTERFACE_TYPE,
    WEIGHTED_NEXTHOPGROUP_MEMBER,
    DEBUG_COUNTER,
    RESOURCE_USAGE_STATS,
    HSDK,
    OBJECT_KEY_CACHE,
    L3_EGRESS_MODE_AUTO_ENABLED,
    SAI_ECN_WRED,
    PKTIO,
    ACL_COPY_TO_CPU,
    SWITCH_ATTR_INGRESS_ACL,
    INGRESS_FIELD_PROCESSOR_FLEX_COUNTER,
    HOSTTABLE,
    PORT_TX_DISABLE,
    ZERO_SDK_WRITE_WARMBOOT,
    OBM_COUNTERS,
    BUFFER_POOL,
    MIRROR_PACKET_TRUNCATION,
    SFLOW_SAMPLING,
    PTP_TC,
    PTP_TC_PCS,
    PENDING_L2_ENTRY,
    MIRROR_V6_TUNNEL,
    EGRESS_QUEUE_FLEX_COUNTER,
    PFC,
    INGRESS_L3_INTERFACE,
  };

  enum class AsicType {
    ASIC_TYPE_FAKE,
    ASIC_TYPE_MOCK,

    ASIC_TYPE_TRIDENT2,
    ASIC_TYPE_TOMAHAWK,
    ASIC_TYPE_TOMAHAWK3,
    ASIC_TYPE_TOMAHAWK4,

    ASIC_TYPE_TAJO,
  };

  virtual ~HwAsic() {}
  virtual bool isSupported(Feature) const = 0;
  virtual AsicType getAsicType() const = 0;
  virtual std::string getVendor() const = 0;
  virtual cfg::PortSpeed getMaxPortSpeed() const = 0;
  virtual std::set<cfg::StreamType> getQueueStreamTypes(bool cpu) const = 0;
  virtual int getDefaultNumPortQueues(cfg::StreamType streamType, bool cpu)
      const = 0;
  virtual uint32_t getMaxLabelStackDepth() const = 0;
  virtual uint64_t getMMUSizeBytes() const = 0;
  virtual uint32_t getMaxMirrors() const = 0;
  virtual uint16_t getMirrorTruncateSize() const = 0;
  virtual uint64_t getDefaultReservedBytes(cfg::StreamType streamType, bool cpu)
      const = 0;
  virtual cfg::MMUScalingFactor getDefaultScalingFactor(
      cfg::StreamType streamType,
      bool cpu) const = 0;
  virtual cfg::PortLoopbackMode desiredLoopbackMode() const {
    return cfg::PortLoopbackMode::MAC;
  }
  virtual bool mmuQgroupsEnabled() const {
    return false;
  }

  /*
    This function will be used in calculating lane assignment, physical port
    assignment when adjusting ports in a port group. Most platforms have
    physical lanes : physical port = 1:1, but due to limited port id, some
    platforms will have different mappings. Note phyiscal port here is referring
    to the chip's physical ports and not our front panel ports
  */
  virtual int getNumLanesPerPhysicalPort() const {
    return 1;
  }

  /*
   * Default Content Aware Processor group ID for ACLs
   */
  virtual int getDefaultACLGroupID() const;

  /*
   * station entry id for vlan interface
   */
  virtual int getStationID(int intfID) const;
  virtual int getDefaultDropEgressID() const;

  /*
   * The maximum number of logical ports
   */
  virtual int getMaxNumLogicalPorts() const = 0;
};

} // namespace facebook::fboss
