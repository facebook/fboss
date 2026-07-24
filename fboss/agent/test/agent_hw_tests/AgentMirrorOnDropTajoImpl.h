// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/test/agent_hw_tests/AgentMirrorOnDropStatelessTest.h"

namespace facebook::fboss {

// Tajo (Yuba, G202X) MirrorOnDrop implementation.
//
// Wire format: outer Eth / IPv6 / UDP / Tajo punt header / inner sample.
// The punt header is parsed by fboss/agent/packet/tajo/TajoPuntHeader.
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
  uint16_t getSrv6MidpointIsLastSidDropReason() const override;
  uint16_t getSrv6DecapNonLastSegmentDropReason() const override;
  uint16_t getSrv6BindingSidNonLastSidDropReason() const override;
  uint16_t getSrv6MidpointUnresolvedDropReason() const override;

  void configureErspanMirror(
      cfg::SwitchConfig& config,
      const std::string& mirrorName,
      const folly::IPAddressV6& tunnelDstIp,
      const folly::IPAddressV6& tunnelSrcIp,
      const PortID& srcPortId) const override;

  ProductionFeature getProductionFeature() const override;

  utility::packetSnooperReceivePacketType snooperReceivePacketType()
      const override {
    return utility::packetSnooperReceivePacketType::PACKET_TYPE_TAJO_MOD;
  }
};

} // namespace facebook::fboss
