/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/BcmFieldProcessorFBConvertors.h"

#include "fboss/agent/FbossError.h"

namespace {
constexpr int kPrioMax = 1e6;
// BCM uses both bcm_l4_port and uint16 to represent icmp type, code
// Hence the templatized version to deal with different pointer types
template <typename BCM_ICMP_TYPE>
void cfgIcmpTypeCodeToBcmIcmpCodeMaskImpl(
    std::optional<uint8_t> type,
    std::optional<uint8_t> code,
    BCM_ICMP_TYPE* bcmCode,
    BCM_ICMP_TYPE* bcmMask) {
  *bcmCode =
      static_cast<BCM_ICMP_TYPE>(static_cast<uint16_t>(type.value()) << 8);
  *bcmMask = 0xFF00;
  if (code) {
    *bcmCode |= static_cast<BCM_ICMP_TYPE>(code.value());
    *bcmMask = 0xFFFF;
  }
}
} // namespace

namespace facebook::fboss::utility {

int swPriorityToHwPriority(int swPrio) {
  return kPrioMax - swPrio;
}

int hwPriorityToSwPriority(int hwPriority) {
  return kPrioMax - hwPriority;
}

bcm_field_IpFrag_t cfgIpFragToBcmIpFrag(cfg::IpFragMatch cfgType) {
  switch (cfgType) {
    case cfg::IpFragMatch::MATCH_NOT_FRAGMENTED:
      return bcmFieldIpFragNon;
    case cfg::IpFragMatch::MATCH_FIRST_FRAGMENT:
      return bcmFieldIpFragFirst;
    case cfg::IpFragMatch::MATCH_NOT_FRAGMENTED_OR_FIRST_FRAGMENT:
      return bcmFieldIpFragNonOrFirst;
    case cfg::IpFragMatch::MATCH_NOT_FIRST_FRAGMENT:
      return bcmFieldIpFragNotFirst;
    case cfg::IpFragMatch::MATCH_ANY_FRAGMENT:
      return bcmFieldIpFragAny;
  }
  // should return in one of the cases
  throw FbossError("Unsupported IP fragment option");
}

cfg::IpFragMatch bcmIpFragToCfgIpFrag(bcm_field_IpFrag_t bcmIpFragType) {
  switch (bcmIpFragType) {
    case bcmFieldIpFragNon:
      return cfg::IpFragMatch::MATCH_NOT_FRAGMENTED;
    case bcmFieldIpFragFirst:
      return cfg::IpFragMatch::MATCH_FIRST_FRAGMENT;
    case bcmFieldIpFragNonOrFirst:
      return cfg::IpFragMatch::MATCH_NOT_FRAGMENTED_OR_FIRST_FRAGMENT;
    case bcmFieldIpFragNotFirst:
      return cfg::IpFragMatch::MATCH_NOT_FIRST_FRAGMENT;
    case bcmFieldIpFragAny:
      return cfg::IpFragMatch::MATCH_ANY_FRAGMENT;
    case bcmFieldIpFragCount:
      // Not represented in config
      break;
  }
  // should return in one of the cases
  throw FbossError("Unsupported IP fragment option");
}

void cfgIcmpTypeCodeToBcmIcmpCodeMask(
    std::optional<uint8_t> type,
    std::optional<uint8_t> code,
    uint16_t* bcmCode,
    uint16_t* bcmMask) {
  cfgIcmpTypeCodeToBcmIcmpCodeMaskImpl(type, code, bcmCode, bcmMask);
}

void cfgIcmpTypeCodeToBcmIcmpCodeMask(
    std::optional<uint8_t> type,
    std::optional<uint8_t> code,
    bcm_l4_port_t* bcmCode,
    bcm_l4_port_t* bcmMask) {
  cfgIcmpTypeCodeToBcmIcmpCodeMaskImpl(type, code, bcmCode, bcmMask);
}

void bcmIcmpTypeCodeToCfgIcmpTypeAndCode(
    uint16_t bcmCode,
    uint16_t bcmMask,
    std::optional<uint8_t>* type,
    std::optional<uint8_t>* code) {
  // Type value is in the higher 8 bits
  *type = bcmCode >> 8;
  if (bcmMask == 0xFFFF) {
    // Mask of 0xFFFF implies we have both type and code
    // Code value is the lower 8 bits
    *code = bcmCode & 0x00FF;
  } else {
    code->reset();
  }
}

void cfgDscpToBcmDscp(uint8_t cfgDscp, uint8* bcmData, uint8* bcmMask) {
  // We shift the supplied value by 2 because brcm looks at the whole
  // tos byte, rather than just the left 6 bits
  // It's nicer to do this here so that our config is still consistent with
  // what's set for arista (so only we have to understand this perculiarity)
  // By shifting we set the 2 ECN bits to 0
  // Set mask to FC so we ignore ECN bits on input
  // TODO: Stop doing this so we can use ECN
  *bcmData = static_cast<uint8>(cfgDscp << 2);
  *bcmMask = 0xFC;
}

void cfgEtherTypeToBcmEtherType(
    cfg::EtherType cfgEtherType,
    uint16* bcmData,
    uint16* bcmMask) {
  *bcmData = static_cast<uint16>(cfgEtherType);
  *bcmMask = 0xFFFF;
}

bcm_field_IpType_t cfgIpTypeToBcmIpType(cfg::IpType cfgIpType) {
  switch (cfgIpType) {
    case cfg::IpType::ANY:
      return bcmFieldIpTypeAny;
    case cfg::IpType::IP:
      return bcmFieldIpTypeIp;
    case cfg::IpType::IP4:
      return bcmFieldIpTypeIpv4Any;
    case cfg::IpType::IP6:
      return bcmFieldIpTypeIpv6;
    case cfg::IpType::ARP_REQUEST:
    case cfg::IpType::ARP_REPLY:
      break;
  }
  // should return in one of the cases
  throw FbossError("Unsupported IP Type option");
}

cfg::IpType bcmIpTypeToCfgIpType(bcm_field_IpType_t bcmIpType) {
  switch (bcmIpType) {
    case bcmFieldIpTypeAny:
      return cfg::IpType::ANY;
    case bcmFieldIpTypeIp:
      return cfg::IpType::IP;
    case bcmFieldIpTypeIpv4Any:
      return cfg::IpType::IP4;
    case bcmFieldIpTypeIpv6:
      return cfg::IpType::IP6;
    default:
      // Unhandled bcm_field_IpType_t values will throw
      break;
  }
  // should return in one of the cases
  throw FbossError("Unsupported IP Type option");
}

bcm_field_stat_t cfgCounterTypeToBcmCounterType(cfg::CounterType cfgType) {
  switch (cfgType) {
    case cfg::CounterType::PACKETS:
      return bcmFieldStatPackets;
    case cfg::CounterType::BYTES:
      return bcmFieldStatBytes;
  }
  // should return in one of the cases
  throw FbossError("Unsupported Counter Type option");
}

cfg::CounterType bcmCounterTypeToCfgCounterType(bcm_field_stat_t bcmType) {
  switch (bcmType) {
    case bcmFieldStatPackets:
      return cfg::CounterType::PACKETS;
    case bcmFieldStatBytes:
      return cfg::CounterType::BYTES;
    default:
      // Unhandled bcm_field_CounterType_t values will throw
      break;
  }
  // should return in one of the cases
  throw FbossError("Unsupported Counter Type option");
}

uint32_t cfgPacketLookupResultToBcmPktResult(
    cfg::PacketLookupResultType cfgPktLookupResult) {
  switch (cfgPktLookupResult) {
    case cfg::PacketLookupResultType::PACKET_LOOKUP_RESULT_MPLS_NO_MATCH:
      return BCM_FIELD_PKT_RES_MPLSUNKNOWN;
  }
  // should return in one of the cases
  throw FbossError("Unsupported PacketLookupResult Type option");
}

cfg::PacketLookupResultType bcmPktResultToCfgPacketLookupResult(
    uint32_t bcmPktLookupResult) {
  switch (bcmPktLookupResult) {
    case BCM_FIELD_PKT_RES_MPLSUNKNOWN:
      return cfg::PacketLookupResultType::PACKET_LOOKUP_RESULT_MPLS_NO_MATCH;
  }
  // should return in one of the cases
  throw FbossError("Unsupported BcmPacketLookupResult Type option");
}

} // namespace facebook::fboss::utility
