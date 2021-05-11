// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/MPLSHandler.h"

#include "fboss/agent/RxPacket.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {
MPLSHandler::MPLSHandler(SwSwitch* sw) : sw_(sw) {}

void MPLSHandler::handlePacket(
    std::unique_ptr<RxPacket> pkt,
    const MPLSHdr& header,
    folly::io::Cursor cursor) {
  if (isLabelProgrammed(header)) {
    return handleKnownLabel(std::move(pkt), header, cursor);
  }
  handleUnknownLabel(std::move(pkt), header, cursor);
}

void MPLSHandler::handleKnownLabel(
    std::unique_ptr<RxPacket> /*pkt*/,
    const MPLSHdr& header,
    folly::io::Cursor /*cursor*/) {
  XLOG_EVERY_MS(ERR, 1000) << "Received Mpls packet with known label:"
                           << header.getLookupLabel();
  // TODO: process and analyze the packet
}

void MPLSHandler::handleUnknownLabel(
    std::unique_ptr<RxPacket> /*pkt*/,
    const MPLSHdr& header,
    folly::io::Cursor /*cursor*/) {
  XLOG_EVERY_MS(ERR, 1000) << "received packet with unknown label:"
                           << header.getLookupLabel();
}

void MPLSHandler::handleLabel2Me(
    std::unique_ptr<RxPacket> /*pkt*/,
    const MPLSHdr& /*header*/,
    folly::io::Cursor /*cursor*/) const {
  // TODO: A label may be punted to CPU for several reasons, one of them could
  // be pop and look up results into cpu next hop; experiment determiend this
  // doesn't strip the label.
}

bool MPLSHandler::isLabelProgrammed(const MPLSHdr& header) const {
  auto lookupLabel = header.getLookupLabel();

  auto labels = sw_->getState()->getLabelForwardingInformationBase();
  if (!labels) {
    return false;
  }
  return labels->getLabelForwardingEntryIf(lookupLabel) != nullptr;
}
} // namespace facebook::fboss
