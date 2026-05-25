// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/test/agent_hw_tests/AgentMirrorOnDropStatelessTest.h"

namespace facebook::fboss {

// Tajo (Yuba=gibraltar, G202X=graphene200) MirrorOnDrop Strategy.
//
// Wire format observed in the MoD sample packet capture:
//   Eth / IPv6 / UDP / Tajo proprietary header (28B) / inner Eth /
//   inner IPv6 / inner transport
//
// Drop reason codes are TBD until Tajo publishes them; placeholders here
// will cause all reason-based test assertions to degenerate to 0x00==0x00.
class TajoMirrorOnDropImpl : public MirrorOnDropImpl {
 public:
  cfg::MirrorOnDropReport makeReport(
      const std::string& name,
      const folly::IPAddressV6& collectorIp,
      int16_t collectorPort,
      int16_t srcPort,
      const folly::IPAddressV6& switchIp,
      std::optional<int32_t> samplingRate) const override;

  MirrorOnDropPacketFields parsePacket(const folly::IOBuf* buf) const override;

  void verifyInvariants(const folly::IOBuf* buf) const override;

  uint16_t getDefaultRouteDropReason() const override;
  uint16_t getAclDropReason() const override;
  uint16_t getMmuDropReason() const override;

  void configureErspanMirror(
      cfg::SwitchConfig& config,
      const std::string& mirrorName,
      const folly::IPAddressV6& tunnelDstIp,
      const folly::IPAddressV6& tunnelSrcIp,
      const PortID& srcPortId) const override;

  ProductionFeature getProductionFeature() const override;
};

} // namespace facebook::fboss
