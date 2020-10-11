// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once
#include "fboss/agent/gen-cpp2/switch_config_types.h"

namespace facebook::fboss {

class HwAsic {
 public:
  enum class Feature {
    BUFFER_PROFILE,
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
    WARM_BOOT,
    QOS_MAP_GLOBAL,
    QCM,
    ROUTE_METADATA,
    SCHEDULER_PPS,
    SMAC_EQUALS_DMAC_CHECK_ENABLED,
    NEXTHOP_TTL_DECREMENT_DISABLE,
    PORT_TTL_DECREMENT_DISABLE,
    PORT_INTERFACE_TYPE,
    L2ENTRY_METADATA,
    WEIGHTED_NEXTHOPGROUP_MEMBER,
    NEIGHBOR_METADATA,
    DEBUG_COUNTER,
    RESOURCE_USAGE_STATS,
    HSDK,
    OBJECT_KEY_CACHE,
    L3_EGRESS_MODE_AUTO_ENABLED,
    SAI_ECN_WRED,
    PKTIO,
    ACL_COPY_TO_CPU,
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
  virtual int getDefaultNumPortQueues(cfg::StreamType streamType) const = 0;
  virtual uint32_t getMaxLabelStackDepth() const = 0;
  virtual uint64_t getMMUSizeBytes() const = 0;
  virtual uint32_t getMaxMirrors() const = 0;
  virtual cfg::PortLoopbackMode desiredLoopbackMode() const {
    return cfg::PortLoopbackMode::MAC;
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
};

} // namespace facebook::fboss
