// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <optional>

#include <fboss/agent/state/Thrifty.h>
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/MirrorOnDropReport.h"
#include "folly/IPAddress.h"

namespace facebook::fboss {

MirrorOnDropReport::MirrorOnDropReport(
    std::string name,
    PortID mirrorPortId,
    folly::IPAddress localSrcIp,
    int16_t localSrcPort,
    folly::IPAddress collectorIp,
    int16_t collectorPort,
    int16_t mtu,
    int16_t truncateSize,
    uint8_t dscp,
    std::optional<int32_t> agingIntervalUsecs,
    std::string switchMac)
    : ThriftStructNode<MirrorOnDropReport, state::MirrorOnDropReportFields>() {
  set<switch_state_tags::name>(name);
  set<switch_state_tags::mirrorPortId>(mirrorPortId);
  set<switch_state_tags::localSrcIp>(network::toBinaryAddress(localSrcIp));
  set<switch_state_tags::localSrcPort>(localSrcPort);
  set<switch_state_tags::collectorIp>(network::toBinaryAddress(collectorIp));
  set<switch_state_tags::collectorPort>(collectorPort);
  set<switch_state_tags::mtu>(mtu);
  set<switch_state_tags::truncateSize>(truncateSize);
  set<switch_state_tags::dscp>(dscp);
  if (agingIntervalUsecs.has_value()) {
    set<switch_state_tags::agingIntervalUsecs>(agingIntervalUsecs.value());
  }
  set<switch_state_tags::switchMac>(switchMac);
}

std::string MirrorOnDropReport::getID() const {
  return get<switch_state_tags::name>()->cref();
}

PortID MirrorOnDropReport::getMirrorPortId() const {
  return PortID(get<switch_state_tags::mirrorPortId>()->cref());
}

void MirrorOnDropReport::setMirrorPortId(PortID portId) {
  set<switch_state_tags::mirrorPortId>(portId);
}

folly::IPAddress MirrorOnDropReport::getLocalSrcIp() const {
  return network::toIPAddress(get<switch_state_tags::localSrcIp>()->toThrift());
}

int16_t MirrorOnDropReport::getLocalSrcPort() const {
  return get<switch_state_tags::localSrcPort>()->cref();
}

folly::IPAddress MirrorOnDropReport::getCollectorIp() const {
  return network::toIPAddress(
      get<switch_state_tags::collectorIp>()->toThrift());
}

int16_t MirrorOnDropReport::getCollectorPort() const {
  return get<switch_state_tags::collectorPort>()->cref();
}

int16_t MirrorOnDropReport::getMtu() const {
  return get<switch_state_tags::mtu>()->cref();
}

int16_t MirrorOnDropReport::getTruncateSize() const {
  return get<switch_state_tags::truncateSize>()->cref();
}

uint8_t MirrorOnDropReport::getDscp() const {
  return get<switch_state_tags::dscp>()->cref();
}

std::optional<uint32_t> MirrorOnDropReport::getAgingIntervalUsecs() const {
  if (auto agingIntervalUsecs = get<switch_state_tags::agingIntervalUsecs>()) {
    return agingIntervalUsecs->cref();
  }
  return std::nullopt;
}

std::string MirrorOnDropReport::getSwitchMac() const {
  return get<switch_state_tags::switchMac>()->cref();
}

template struct ThriftStructNode<
    MirrorOnDropReport,
    state::MirrorOnDropReportFields>;

} // namespace facebook::fboss
