// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/show/route/utils.h"

namespace facebook::fboss::show::route::utils {

using facebook::fboss::NextHopThrift;
using facebook::fboss::utils::getAddrStr;

std::string getMplsActionCodeStr(MplsActionCode mplsActionCode) {
  switch (mplsActionCode) {
    case MplsActionCode::PUSH:
      return "PUSH";
    case MplsActionCode::SWAP:
      return "SWAP";
    case MplsActionCode::PHP:
      return "PHP";
    case MplsActionCode::POP_AND_LOOKUP:
      return "POP_AND_LOOKUP";
    case MplsActionCode::NOOP:
      return "NOOP";
  }
  throw std::runtime_error(
      "Unsupported MplsActionCode: " +
      std::to_string(static_cast<int>(mplsActionCode)));
}

std::string getMplsActionInfoStr(const cli::MplsActionInfo& mplsActionInfo) {
  const auto& action = mplsActionInfo.action().value();
  auto swapLabelPtr = apache::thrift::get_pointer(mplsActionInfo.swapLabel());
  auto pushLabelsPtr = apache::thrift::get_pointer(mplsActionInfo.pushLabels());
  std::string labels;

  if (action == "SWAP" && swapLabelPtr != nullptr) {
    labels = fmt::format(": {}", *swapLabelPtr);
  } else if (action == "PUSH" && pushLabelsPtr != nullptr) {
    auto stackStr = folly::join(",", *pushLabelsPtr);
    labels = fmt::format(": {{{}}}", stackStr);
  }
  return fmt::format(" MPLS -> {} {}", mplsActionInfo.action().value(), labels);
}

void getNextHopInfoAddr(
    const network::thrift::BinaryAddress& addr,
    cli::NextHopInfo& nextHopInfo) {
  nextHopInfo.addr() = getAddrStr(addr);
  auto ifNamePtr = apache::thrift::get_pointer(addr.ifName());
  if (ifNamePtr != nullptr) {
    nextHopInfo.ifName() = *ifNamePtr;
  }
}

void getNextHopInfoThrift(
    const NextHopThrift& nextHop,
    cli::NextHopInfo& nextHopInfo) {
  getNextHopInfoAddr(nextHop.address().value(), nextHopInfo);
  nextHopInfo.weight() = folly::copy(nextHop.weight().value());

  auto mplsActionPtr = apache::thrift::get_pointer(nextHop.mplsAction());
  if (mplsActionPtr != nullptr) {
    cli::MplsActionInfo mplsActionInfo;
    mplsActionInfo.action() =
        getMplsActionCodeStr(folly::copy(mplsActionPtr->action().value()));
    auto swapLabelPtr = apache::thrift::get_pointer(mplsActionPtr->swapLabel());
    auto pushLabelsPtr =
        apache::thrift::get_pointer(mplsActionPtr->pushLabels());
    if (swapLabelPtr != nullptr) {
      mplsActionInfo.swapLabel() = *swapLabelPtr;
    }
    if (pushLabelsPtr != nullptr) {
      mplsActionInfo.pushLabels() = *pushLabelsPtr;
    }
    nextHopInfo.mplsAction() = mplsActionInfo;
  }

  auto topologyInfoPtr = apache::thrift::get_pointer(nextHop.topologyInfo());
  if (topologyInfoPtr != nullptr) {
    nextHopInfo.topologyInfo() = *topologyInfoPtr;
  }
}

std::string getMplsLabelStr(const cli::NextHopInfo& nextHopInfo) {
  std::string labelStr;
  auto mplsActionInfoPtr =
      apache::thrift::get_pointer(nextHopInfo.mplsAction());
  if (mplsActionInfoPtr != nullptr) {
    labelStr = getMplsActionInfoStr(*mplsActionInfoPtr);
  }
  return labelStr;
}
std::string getTopologyInfoStr(const cli::NextHopInfo& nextHopInfo) {
  std::string topoStr;
  auto topoInfoPtr = apache::thrift::get_pointer(nextHopInfo.topologyInfo());
  if (topoInfoPtr != nullptr) {
    std::string rackCapacityStr = "none";
    if (topoInfoPtr->local_rack_capacity().has_value()) {
      rackCapacityStr =
          std::to_string(topoInfoPtr->local_rack_capacity().value());
    }
    std::string remoteRackCapacityStr = "none";
    if (topoInfoPtr->remote_rack_capacity().has_value()) {
      remoteRackCapacityStr =
          std::to_string(topoInfoPtr->remote_rack_capacity().value());
    }
    std::string spineCapacityStr = "none";
    if (topoInfoPtr->spine_capacity().has_value()) {
      spineCapacityStr = std::to_string(topoInfoPtr->spine_capacity().value());
    }
    std::string planeStr = topoInfoPtr->plane_id().has_value()
        ? std::to_string(topoInfoPtr->plane_id().value())
        : "none";
    std::string rackStr = topoInfoPtr->rack_id().has_value()
        ? std::to_string(topoInfoPtr->rack_id().value())
        : "none";
    topoStr = fmt::format(
        " rack {} plane {} remote weight {} spine weight {} local weight {}",
        rackStr,
        planeStr,
        remoteRackCapacityStr,
        spineCapacityStr,
        rackCapacityStr);
  }
  return topoStr;
}

std::string getInterfaceIDStr(const cli::NextHopInfo& nextHopInfo) {
  std::string interfaceIDStr;
  auto interfaceIDPtr = apache::thrift::get_pointer(nextHopInfo.interfaceID());
  if (interfaceIDPtr != nullptr) {
    interfaceIDStr = fmt::format("(i/f {}) ", *interfaceIDPtr);
  }
  return interfaceIDStr;
}
std::string getWeightStr(const cli::NextHopInfo& nextHopInfo) {
  std::string weightStr;
  if (folly::copy(nextHopInfo.weight().value())) {
    weightStr =
        fmt::format(" weight {}", folly::copy(nextHopInfo.weight().value()));
  }
  return weightStr;
}

std::string getNextHopInfoStr(const cli::NextHopInfo& nextHopInfo) {
  auto ifNamePtr = apache::thrift::get_pointer(nextHopInfo.ifName());
  std::string viaStr;
  if (ifNamePtr != nullptr) {
    viaStr = fmt::format(" dev {}", *ifNamePtr);
  }
  std::string labelStr = getMplsLabelStr(nextHopInfo);
  std::string interfaceIDStr = getInterfaceIDStr(nextHopInfo);
  std::string weightStr = getWeightStr(nextHopInfo);
  std::string topologyStr = getTopologyInfoStr(nextHopInfo);
  auto ret = fmt::format(
      "{}{}{}{}{}{}",
      interfaceIDStr,
      nextHopInfo.addr().value(),
      viaStr,
      weightStr,
      labelStr,
      topologyStr);
  return ret;
}

std::string getViaStr(
    const std::basic_string<char>* ifNamePtr,
    const std::map<std::string, std::string>& vlanAggregatePortMap,
    const std::map<
        std::string,
        std::map<std::string, std::vector<std::string>>>& vlanPortMap) {
  std::string fbossStr = "fboss";
  std::string viaStr;
  if (ifNamePtr != nullptr) {
    std::string vlanId = *ifNamePtr;
    if (vlanId.rfind(fbossStr, 0) == 0) {
      vlanId = vlanId.substr(fbossStr.length());
    }
    if (vlanAggregatePortMap.find(vlanId) != vlanAggregatePortMap.end()) {
      viaStr = fmt::format(" dev {}", vlanAggregatePortMap.at(vlanId));
    } else if (vlanPortMap.find(vlanId) != vlanPortMap.end()) {
      std::vector<std::string> port_names;
      for (const auto& ports : vlanPortMap.at(vlanId)) {
        for (const auto& port : ports.second) {
          port_names.push_back(port);
        }
      }
      if (!port_names.empty()) {
        viaStr = fmt::format(" dev {}", folly::join(", ", port_names));
      }
    } else {
      viaStr = fmt::format(" dev {}", *ifNamePtr);
    }
  }
  return viaStr;
}

std::string getNextHopInfoStr(
    const cli::NextHopInfo& nextHopInfo,
    const std::map<std::string, std::string>& vlanAggregatePortMap,
    const std::map<
        std::string,
        std::map<std::string, std::vector<std::string>>>& vlanPortMap) {
  auto ifNamePtr = apache::thrift::get_pointer(nextHopInfo.ifName());
  auto viaStr = getViaStr(ifNamePtr, vlanAggregatePortMap, vlanPortMap);
  std::string labelStr = getMplsLabelStr(nextHopInfo);
  std::string interfaceIDStr = getInterfaceIDStr(nextHopInfo);
  std::string weightStr = getWeightStr(nextHopInfo);
  auto ret = fmt::format(
      "{}{}{}{}{}",
      interfaceIDStr,
      nextHopInfo.addr().value(),
      viaStr,
      weightStr,
      labelStr);
  return ret;
}

} // namespace facebook::fboss::show::route::utils
