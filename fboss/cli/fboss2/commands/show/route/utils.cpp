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
  const auto& action = mplsActionInfo.get_action();
  auto swapLabelPtr = mplsActionInfo.get_swapLabel();
  auto pushLabelsPtr = mplsActionInfo.get_pushLabels();
  std::string labels;

  if (action == "SWAP" && swapLabelPtr != nullptr) {
    labels = fmt::format(": {}", *swapLabelPtr);
  } else if (action == "PUSH" && pushLabelsPtr != nullptr) {
    auto stackStr = folly::join(",", *pushLabelsPtr);
    labels = fmt::format(": {{{}}}", stackStr);
  }
  return fmt::format(" MPLS -> {} {}", mplsActionInfo.get_action(), labels);
}

void getNextHopInfoAddr(
    const network::thrift::BinaryAddress& addr,
    cli::NextHopInfo& nextHopInfo) {
  nextHopInfo.addr() = getAddrStr(addr);
  auto ifNamePtr = addr.get_ifName();
  if (ifNamePtr != nullptr) {
    nextHopInfo.ifName() = *ifNamePtr;
  }
}

void getNextHopInfoThrift(
    const NextHopThrift& nextHop,
    cli::NextHopInfo& nextHopInfo) {
  getNextHopInfoAddr(nextHop.get_address(), nextHopInfo);
  nextHopInfo.weight() = nextHop.get_weight();

  auto mplsActionPtr = nextHop.get_mplsAction();
  if (mplsActionPtr != nullptr) {
    cli::MplsActionInfo mplsActionInfo;
    mplsActionInfo.action() = getMplsActionCodeStr(mplsActionPtr->get_action());
    auto swapLabelPtr = mplsActionPtr->get_swapLabel();
    auto pushLabelsPtr = mplsActionPtr->get_pushLabels();
    if (swapLabelPtr != nullptr) {
      mplsActionInfo.swapLabel() = *swapLabelPtr;
    }
    if (pushLabelsPtr != nullptr) {
      mplsActionInfo.pushLabels() = *pushLabelsPtr;
    }
    nextHopInfo.mplsAction() = mplsActionInfo;
  }
}

std::string getNextHopInfoStr(const cli::NextHopInfo& nextHopInfo) {
  auto ifNamePtr = nextHopInfo.get_ifName();
  std::string viaStr;
  if (ifNamePtr != nullptr) {
    viaStr = fmt::format(" dev {}", *ifNamePtr);
  }
  std::string labelStr;
  auto mplsActionInfoPtr = nextHopInfo.get_mplsAction();
  if (mplsActionInfoPtr != nullptr) {
    labelStr = getMplsActionInfoStr(*mplsActionInfoPtr);
  }
  auto interfaceIDPtr = nextHopInfo.get_interfaceID();
  std::string interfaceIDStr;
  if (interfaceIDPtr != nullptr) {
    interfaceIDStr = fmt::format("(i/f {}) ", *interfaceIDPtr);
  }
  std::string weightStr;
  if (nextHopInfo.get_weight()) {
    weightStr = fmt::format(" weight {}", nextHopInfo.get_weight());
  }
  auto ret = fmt::format(
      "{}{}{}{}{}",
      interfaceIDStr,
      nextHopInfo.get_addr(),
      viaStr,
      weightStr,
      labelStr);
  return ret;
}

} // namespace facebook::fboss::show::route::utils
