// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/types.h"

#include <folly/IPAddress.h>

namespace facebook::fboss::utility {

template <typename AddrT>
struct MirrorTestParams {
  AddrT senderIp;
  AddrT receiverIp;
  AddrT mirrorDestinationIp;

  MirrorTestParams(
      const AddrT& _senderIp,
      const AddrT& _receiverIp,
      const AddrT& _mirrorDestinationIp)
      : senderIp(_senderIp),
        receiverIp(_receiverIp),
        mirrorDestinationIp(_mirrorDestinationIp) {}
};

template <typename AddrT>
MirrorTestParams<AddrT> getMirrorTestParams();

inline const std::string kIngressSpan("ingress_span");
inline const std::string kIngressErspan("ingress_erspan");
inline const std::string kEgressSpan("egress_span");
inline const std::string kEgressErspan("egress_erspan");

// Port 0 is used for traffic and port 1 is used for mirroring.
inline const uint8_t kTrafficPortIndex = 0;
inline const uint8_t kMirrorToPortIndex = 1;
inline const uint8_t kSflowToPortIndex = 2;

constexpr auto kDscpDefault = facebook::fboss::cfg::switch_config_constants::
    DEFAULT_MIRROR_DSCP_; // default dscp value

template <typename AddrT>
void addMirrorConfig(
    cfg::SwitchConfig* cfg,
    const AgentEnsemble& ensemble,
    const std::string& mirrorName,
    bool truncate,
    uint8_t dscp = kDscpDefault);

folly::IPAddress getSflowMirrorDestination(bool isV4);

void configureSflowMirror(
    cfg::SwitchConfig& config,
    const std::string& mirrorName,
    bool truncate,
    const std::string& destinationIp,
    uint32_t udpSrcPort = 6343);

void configureSflowSampling(
    cfg::SwitchConfig& config,
    const std::string& mirrorName,
    const std::vector<PortID>& ports,
    int sampleRate);

void configureTrapAcl(cfg::SwitchConfig& config, bool isV4);
void configureTrapAcl(cfg::SwitchConfig& config, PortID portId);

} // namespace facebook::fboss::utility
