// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/utils/UdfTestUtils.h"

namespace facebook::fboss::utility {

static cfg::UdfConfig addUdfConfig(
    std::map<std::string, cfg::UdfGroup>& udfMap,
    const std::string& udfGroup,
    const int offsetBytes,
    const int fieldSizeBytes,
    cfg::UdfGroupType udfType) {
  cfg::UdfConfig udfCfg;
  cfg::UdfGroup udfGroupEntry;
  cfg::UdfPacketMatcher matchCfg;
  std::map<std::string, cfg::UdfPacketMatcher> udfPacketMatcherMap;

  matchCfg.name() = kUdfL4UdpRocePktMatcherName;
  matchCfg.l4PktType() = cfg::UdfMatchL4Type::UDF_L4_PKT_TYPE_UDP;
  matchCfg.UdfL4DstPort() = kUdfL4DstPort;

  udfGroupEntry.name() = udfGroup;
  udfGroupEntry.header() = cfg::UdfBaseHeaderType::UDF_L4_HEADER;
  udfGroupEntry.startOffsetInBytes() = offsetBytes;
  udfGroupEntry.fieldSizeInBytes() = fieldSizeBytes;
  // has to be the same as in matchCfg
  udfGroupEntry.udfPacketMatcherIds() = {kUdfL4UdpRocePktMatcherName};
  udfGroupEntry.type() = udfType;

  udfMap.insert(std::make_pair(*udfGroupEntry.name(), udfGroupEntry));
  udfPacketMatcherMap.insert(std::make_pair(*matchCfg.name(), matchCfg));
  udfCfg.udfGroups() = udfMap;
  udfCfg.udfPacketMatcher() = udfPacketMatcherMap;
  return udfCfg;
}

cfg::UdfConfig addUdfAclConfig(int udfType) {
  std::map<std::string, cfg::UdfGroup> udfMap;
  cfg::UdfConfig udfCfg;
  if ((udfType & kUdfOffsetBthOpcode) == kUdfOffsetBthOpcode) {
    udfCfg = addUdfConfig(
        udfMap,
        kUdfAclRoceOpcodeGroupName,
        kUdfAclRoceOpcodeStartOffsetInBytes,
        kUdfAclRoceOpcodeFieldSizeInBytes,
        cfg::UdfGroupType::ACL);
  }
  if ((udfType & kUdfOffsetBthReserved) == kUdfOffsetBthReserved) {
    udfCfg = addUdfConfig(
        udfMap,
        kRoceUdfFlowletGroupName,
        kRoceUdfFlowletStartOffsetInBytes,
        kRoceUdfFlowletFieldSizeInBytes,
        cfg::UdfGroupType::ACL);
  }
  if ((udfType & kUdfOffsetAethSyndrome) == kUdfOffsetAethSyndrome) {
    udfCfg = addUdfConfig(
        udfMap,
        kUdfAclAethNakGroupName,
        kUdfAclAethNakStartOffsetInBytes,
        kUdfAclAethNakFieldSizeInBytes,
        cfg::UdfGroupType::ACL);
  }
  if ((udfType & kUdfOffsetRethDmaLength) == kUdfOffsetRethDmaLength) {
    udfCfg = addUdfConfig(
        udfMap,
        kUdfAclRethWrImmZeroGroupName,
        kUdfAclRethDmaLenOffsetInBytes,
        kUdfAclRethDmaLenFieldSizeInBytes,
        cfg::UdfGroupType::ACL);
  }
  return udfCfg;
}

cfg::UdfConfig addUdfHashConfig(cfg::AsicType asicType) {
  std::map<std::string, cfg::UdfGroup> udfMap;
  if (asicType == cfg::AsicType::ASIC_TYPE_CHENAB) {
    return addUdfConfig(
        udfMap,
        kUdfHashDstQueuePairGroupName,
        kChenabUdfHashDstQueuePairStartOffsetInBytes,
        kChenabUdfHashDstQueuePairFieldSizeInBytes,
        cfg::UdfGroupType::HASH);
  }
  return addUdfConfig(
      udfMap,
      kUdfHashDstQueuePairGroupName,
      kUdfHashDstQueuePairStartOffsetInBytes,
      kUdfHashDstQueuePairFieldSizeInBytes,
      cfg::UdfGroupType::HASH);
}

cfg::UdfConfig addUdfHashAclConfig(cfg::AsicType asicType) {
  std::map<std::string, cfg::UdfGroup> udfMap;
  if (asicType == cfg::AsicType::ASIC_TYPE_CHENAB) {
    addUdfConfig(
        udfMap,
        kUdfHashDstQueuePairGroupName,
        kChenabUdfHashDstQueuePairStartOffsetInBytes,
        kChenabUdfHashDstQueuePairFieldSizeInBytes,
        cfg::UdfGroupType::HASH);

  } else {
    addUdfConfig(
        udfMap,
        kUdfHashDstQueuePairGroupName,
        kUdfHashDstQueuePairStartOffsetInBytes,
        kUdfHashDstQueuePairFieldSizeInBytes,
        cfg::UdfGroupType::HASH);
  }
  return addUdfConfig(
      udfMap,
      kUdfAclRoceOpcodeGroupName,
      kUdfAclRoceOpcodeStartOffsetInBytes,
      kUdfAclRoceOpcodeFieldSizeInBytes,
      cfg::UdfGroupType::ACL);
}

} // namespace facebook::fboss::utility
