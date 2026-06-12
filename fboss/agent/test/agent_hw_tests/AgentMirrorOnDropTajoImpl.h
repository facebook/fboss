// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/test/agent_hw_tests/AgentMirrorOnDropStatelessTest.h"

namespace facebook::fboss {

// Tajo (Yuba, G202X) MirrorOnDrop implementation.
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

  uint16_t expectedIngressPort(const HwSwitch* hw, const PortID& portId)
      const override;

  void verifyInvariants(const folly::IOBuf* buf) const override;

  MirrorOnDropDropReasonCodes packMmuDropReason(uint16_t code) const override;

  uint16_t getDefaultRouteDropReason() const override;
  uint16_t getAclDropReason() const override;
  uint16_t getMmuDropReason() const override;
  uint16_t getSrv6MidpointNonLastSidDropReason() const override;
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
};

} // namespace facebook::fboss
