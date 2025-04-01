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
}

std::string getNextHopInfoStr(const cli::NextHopInfo& nextHopInfo) {
  auto ifNamePtr = apache::thrift::get_pointer(nextHopInfo.ifName());
  std::string viaStr;
  if (ifNamePtr != nullptr) {
    viaStr = fmt::format(" dev {}", *ifNamePtr);
  }
  std::string labelStr;
  auto mplsActionInfoPtr =
      apache::thrift::get_pointer(nextHopInfo.mplsAction());
  if (mplsActionInfoPtr != nullptr) {
    labelStr = getMplsActionInfoStr(*mplsActionInfoPtr);
  }
  auto interfaceIDPtr = apache::thrift::get_pointer(nextHopInfo.interfaceID());
  std::string interfaceIDStr;
  if (interfaceIDPtr != nullptr) {
    interfaceIDStr = fmt::format("(i/f {}) ", *interfaceIDPtr);
  }
  std::string weightStr;
  if (folly::copy(nextHopInfo.weight().value())) {
    weightStr =
        fmt::format(" weight {}", folly::copy(nextHopInfo.weight().value()));
  }
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
