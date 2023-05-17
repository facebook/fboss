// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once
#include <fboss/lib/phy/gen-cpp2/phy_types.h>
#include <folly/MacAddress.h>
#include "fboss/agent/gen-cpp2/switch_config_types.h"

namespace facebook::fboss {

class HwAsic {
 public:
  HwAsic(
      cfg::SwitchType switchType,
      std::optional<int64_t> switchId,
      std::optional<cfg::Range64> systemPortRange,
      folly::MacAddress& mac,
      std::unordered_set<cfg::SwitchType> supportedModes = {
          cfg::SwitchType::NPU});
  enum class Feature {
    SPAN,
    ERSPANv4,
    ERSPANv6,
    SFLOWv4,
    SFLOWv6,
    MPLS,
    MPLS_ECMP,
    SAI_MPLS_QOS,
    HASH_FIELDS_CUSTOMIZATION,
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
    EGRESS_QUEUE_FLEX_COUNTER,
    PFC,
    INGRESS_L3_INTERFACE,
    NON_UNICAST_HASH,
    DETAILED_L2_UPDATE,
    COUNTER_REFRESH_INTERVAL,
    TELEMETRY_AND_MONITORING,
    WIDE_ECMP,
    ALPM_ROUTE_PROJECTION,
    MAC_AGING,
    SAI_PORT_SPEED_CHANGE,
    SFLOW_SHIM_VERSION_FIELD,
    REMOVE_PORTS_FOR_COLDBOOT,
    EGRESS_MIRRORING,
    EGRESS_SFLOW,
    DEFAULT_VLAN,
    SAI_LAG_HASH,
    L2_LEARNING,
    SAI_ACL_ENTRY_SRC_PORT_QUALIFIER,
    TRAFFIC_HASHING,
    ACL_TABLE_GROUP,
    MACSEC,
    CPU_PORT,
    VRF,
    SAI_HASH_FIELDS_CLEAR_BEFORE_SET,
    EMPTY_ACL_MATCHER,
    SAI_PORT_SERDES_FIELDS_RESET,
    ROUTE_COUNTERS,
    MULTIPLE_ACL_TABLES,
    ROUTE_FLEX_COUNTERS,
    BRIDGE_PORT_8021Q,
    FEC_DIAG_COUNTERS,
    SAI_WEIGHTED_NEXTHOPGROUP_MEMBER,
    SAI_ACL_TABLE_UPDATE,
    PORT_EYE_VALUES,
    SAI_MPLS_TTL_1_TRAP,
    SAI_MPLS_LABEL_LOOKUP_FAIL_COUNTER,
    PMD_RX_LOCK_STATUS,
    PMD_RX_SIGNAL_DETECT,
    SAI_FEC_COUNTERS,
    SAI_PORT_ERR_STATUS,
    EXACT_MATCH,
    ROUTE_PROGRAMMING,
    ECMP_HASH_V4,
    ECMP_HASH_V6,
    FEC_CORRECTED_BITS,
    MEDIA_TYPE,
    FEC,
    RX_FREQUENCY_PPM,
    FABRIC_PORTS,
    ECMP_MEMBER_WIDTH_INTROSPECTION,
    FABRIC_PORT_MTU,
    SAI_FIRMWARE_PATH,
    EXTENDED_FEC,
    LINK_TRAINING,
    SAI_RX_REASON_COUNTER,
    SAI_MPLS_INSEGMENT,
    RESERVED_ENCAP_INDEX_RANGE,
    VOQ,
    RECYCLE_PORTS,
    XPHY_PORT_STATE_TOGGLE,
    SAI_PORT_GET_PMD_LANES,
    FABRIC_TX_QUEUES,
    SAI_PORT_VCO_CHANGE,
    SAI_TTL0_PACKET_FORWARD_ENABLE,
    WARMBOOT,
    SHARED_INGRESS_EGRESS_BUFFER_POOL,
    ROUTE_METADATA,
    DLB,
    P4_WARMBOOT,
    IN_PAUSE_INCREMENTS_DISCARDS,
    FEC_AM_LOCK_STATUS,
    PCS_RX_LINK_STATUS,
    TC_TO_QUEUE_QOS_MAP_ON_SYSTEM_PORT,
    SAI_CONFIGURE_SIX_TAP,
    UDF_HASH_FIELD_QUERY,
    SAI_SAMPLEPACKET_TRAP,
    PORT_FABRIC_ISOLATE,
    QUEUE_ECN_COUNTER,
    TRAP_PRIORITY_LOWER_VAL_IS_LOWER_PRI,
    CPU_TX_VIA_RECYCLE_PORT,
    QUEUE_PRIORITY_LOWER_VAL_IS_HIGH_PRI,
    SAI_CONFIGURE_SEVEN_TAP,
    SWITCH_DROP_STATS,
    SAI_UDF_HASH,
    INGRESS_PRIORITY_GROUP_HEADROOM_WATERMARK,
  };

  enum class AsicMode {
    ASIC_MODE_SIM,
    ASIC_MODE_HW,
  };

  enum class AsicVendor {
    ASIC_VENDOR_BCM,
    ASIC_VENDOR_TAJO,
    ASIC_VENDOR_CREDO,
    ASIC_VENDOR_MARVELL,
    ASIC_VENDOR_MOCK,
    ASIC_VENDOR_FAKE,
  };
  virtual ~HwAsic() {}
  static std::unique_ptr<HwAsic> makeAsic(
      cfg::AsicType asicType,
      cfg::SwitchType switchType,
      std::optional<int64_t> switchID,
      std::optional<cfg::Range64> systemPortRange,
      folly::MacAddress& mac);
  virtual bool isSupported(Feature) const = 0;
  virtual cfg::AsicType getAsicType() const = 0;
  std::string getAsicTypeStr() const;
  virtual AsicVendor getAsicVendor() const = 0;
  virtual AsicMode getAsicMode() const {
    return AsicMode::ASIC_MODE_HW;
  }
  virtual phy::DataPlanePhyChipType getDataPlanePhyChipType() const = 0;
  virtual std::string getVendor() const = 0;
  virtual cfg::PortSpeed getMaxPortSpeed() const = 0;
  virtual std::set<cfg::StreamType> getQueueStreamTypes(
      cfg::PortType portType) const = 0;
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
  // Get the smallest packet buffer unit for ASIC, cell size for BCM
  virtual uint32_t getPacketBufferUnitSize() const = 0;

  // Get the size of the buffer descriptor associated with each packet
  virtual uint32_t getPacketBufferDescriptorSize() const = 0;

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
   * Default Content Aware Processor group ID for TeFlows
   */
  virtual int getDefaultTeFlowGroupID() const;

  /*
   * station entry id for vlan interface
   */
  virtual int getStationID(int intfID) const;
  virtual int getDefaultDropEgressID() const;

  /*
   * The maximum number of logical ports
   */
  virtual int getMaxNumLogicalPorts() const = 0;
  virtual uint32_t getMaxWideEcmpSize() const = 0;

  /*
   * The maximum number of lag member
   */
  virtual uint32_t getMaxLagMemberSize() const = 0;

  /*
   * Port ID offset for system port. Default is 0.
   * This will be added to PortID and will be carried in the
   * sflow shim header
   */
  virtual int getSystemPortIDOffset() const {
    return 0;
  }

  virtual uint32_t getNumCores() const = 0;

  virtual uint32_t getSflowShimHeaderSize() const = 0;

  virtual std::optional<uint32_t> getPortSerdesPreemphasis() const = 0;

  static std::vector<cfg::AsicType> getAllHwAsicList();

  virtual uint32_t getMaxVariableWidthEcmpSize() const = 0;

  virtual uint32_t getMaxEcmpSize() const = 0;

  virtual bool scalingFactorBasedDynamicThresholdSupported() const = 0;

  virtual int getBufferDynThreshFromScalingFactor(
      cfg::MMUScalingFactor scalingFactor) const = 0;

  virtual uint32_t getStaticQueueLimitBytes() const = 0;

  virtual cfg::Range64 getReservedEncapIndexRange() const;

  virtual uint32_t getNumMemoryBuffers() const = 0;

  cfg::SwitchType getSwitchType() const {
    return switchType_;
  }
  std::optional<uint64_t> getSwitchId() const {
    return switchId_;
  }
  std::optional<cfg::Range64> getSystemPortRange() const {
    return systemPortRange_;
  }

  cfg::StreamType getDefaultStreamType() const {
    return defaultStreamType_;
  }

  void setDefaultStreamType(cfg::StreamType streamType) {
    defaultStreamType_ = streamType;
  }

  const folly::MacAddress& getAsicMac() const {
    return asicMac_;
  }

  struct RecyclePortInfo {
    uint32_t coreId;
    uint32_t corePortIndex;
    uint32_t speedMbps;
  };

  virtual RecyclePortInfo getRecyclePortInfo() const;

 protected:
  static cfg::Range64 makeRange(int64_t min, int64_t max);

 private:
  cfg::SwitchType switchType_;
  std::optional<int64_t> switchId_;
  std::optional<cfg::Range64> systemPortRange_;
  cfg::StreamType defaultStreamType_{cfg::StreamType::ALL};
  folly::MacAddress asicMac_;
};

} // namespace facebook::fboss
