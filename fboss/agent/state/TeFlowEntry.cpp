// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/state/TeFlowEntry.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

std::shared_ptr<TeFlowEntry> TeFlowEntry::createTeFlowEntry(
    const FlowEntry& entry) {
  auto teFlowEntry = std::make_shared<TeFlowEntry>(*entry.flow());
  teFlowEntry->setNextHops(*entry.nextHops());
  if (entry.counterID().has_value()) {
    teFlowEntry->setCounterID(entry.counterID().value());
  }
  return teFlowEntry;
}

std::string TeFlowEntry::getID() const {
  return getTeFlowStr(getFlow()->toThrift());
}

template <typename AddrT>
std::optional<folly::MacAddress> TeFlowEntry::getNeighborMac(
    const std::shared_ptr<SwitchState>& state,
    const std::shared_ptr<Interface>& interface,
    AddrT ip) {
  auto vlan = state->getVlans()->getVlanIf(interface->getVlanID());
  auto neighbor = vlan->getNeighborEntryTable<AddrT>()->getNodeIf(ip.str());
  if (!neighbor) {
    return std::nullopt;
  }
  return neighbor->getMac();
}

bool TeFlowEntry::isNexthopResolved(
    NextHopThrift nexthop,
    const std::shared_ptr<SwitchState>& state) {
  auto nhop = util::fromThrift(nexthop, true);
  if (!nhop.isResolved()) {
    XLOG(WARNING) << "Unresolved nexthop for TE flow " << nhop;
    return false;
  }
  auto interface = state->getInterfaces()->getNodeIf(nhop.intfID().value());
  std::optional<folly::MacAddress> dstMac;
  if (nhop.addr().isV6()) {
    dstMac = getNeighborMac<folly::IPAddressV6>(
        state, interface, nhop.addr().asV6());
    if (!dstMac.has_value()) {
      XLOG(WARNING) << "No NDP entry for TE flow redirection nexthop " << nhop;
      return false;
    }
  } else {
    dstMac = getNeighborMac<folly::IPAddressV4>(
        state, interface, nhop.addr().asV4());
    if (!dstMac.has_value()) {
      XLOG(WARNING) << "No ARP entry for TE flow redirection nexthop " << nhop;
      return false;
    }
  }
  if (dstMac.value().isBroadcast()) {
    XLOG(WARNING)
        << "No resolved neighbor entry for TE flow redirection nexthop "
        << nhop;
    return false;
  }
  return true;
}

void TeFlowEntry::resolve(const std::shared_ptr<SwitchState>& state) {
  std::vector<NextHopThrift> resolvedNextHops;
  for (const auto& nexthop : getNextHops()->toThrift()) {
    if (!isNexthopResolved(nexthop, state)) {
      auto nhop = util::fromThrift(nexthop, true);
      throw FbossError(
          "Invalid redirection nexthop. NH: ",
          nhop.str(),
          " TeFlow entry: ",
          getID());
    }
    resolvedNextHops.emplace_back(nexthop);
  }
  setResolvedNextHops(std::move(resolvedNextHops));
  setEnabled(true);
}

std::string TeFlowEntry::str() const {
  std::string flowString{};
  auto flow = getFlow()->toThrift();
  if (!flow.dstPrefix().has_value() || !flow.srcPort().has_value()) {
    return "invalid";
  }
  auto prefix = flow.dstPrefix().value();
  folly::IPAddress ipaddr = network::toIPAddress(*prefix.ip());
  flowString.append(fmt::format(
      "dstPrefix:{}/{},srcPort:{}",
      ipaddr.str(),
      *prefix.prefixLength(),
      *flow.srcPort()));
  auto counter = getCounterID();
  flowString.append(
      fmt::format(",counterID:{}", counter ? counter->toThrift() : "null"));
  flowString.append(fmt::format(",Nexthop:"));
  const auto& nhops = getNextHops();
  for (const auto& nhop : util::toRouteNextHopSet(nhops->toThrift())) {
    flowString.append(fmt::format("{},", nhop.str()));
  }
  flowString.append(fmt::format("ResolvedNexthop:"));
  const auto& resolvedNhops = getResolvedNextHops();
  for (const auto& nhop : util::toRouteNextHopSet(resolvedNhops->toThrift())) {
    flowString.append(fmt::format("{},", nhop.str()));
  }
  flowString.append(fmt::format("Enabled:{}", getEnabled()));
  return flowString;
}

TeFlowDetails TeFlowEntry::toDetails() const {
  TeFlowDetails details{};
  details.flow() = getFlow()->toThrift();
  details.enabled() = getEnabled();
  details.nexthops() = getNextHops()->toThrift();
  details.resolvedNexthops() = getResolvedNextHops()->toThrift();
  details.counterID() = "null";
  if (const auto& counter = getCounterID()) {
    details.counterID() = counter->toThrift();
  }
  return details;
}
} // namespace facebook::fboss
