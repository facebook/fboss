// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/MPLSHandler.h"

#include "fboss/agent/RxPacket.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/packet/MPLSHdr.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/state/SwitchState.h"
#include "folly/io/Cursor.h"

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
    std::unique_ptr<RxPacket> pkt,
    const MPLSHdr& header,
    folly::io::Cursor cursor) {
  auto topLabel = header.getLookupLabel();
  XLOG(WARNING) << "Received Mpls packet with known label:"
                << topLabel.getLabelValue();
  auto entry =
      sw_->getState()->getMultiLabelForwardingInformationBase()->getNode(
          topLabel.getLabelValue());
  const auto& fwd = entry->getForwardInfo();

  if (fwd.getAction() == LabelNextHopEntry::Action::TO_CPU) {
    return handleLabel2Me(std::move(pkt), header, cursor);
  }
  if (entry->isPopAndLookup()) {
    return popLabelAndLookup(std::move(pkt), header, cursor);
  }
  // ignore any packet which is not pop and look up
  XLOG_EVERY_MS(ERR, 1000) << "dropping known label: "
                           << topLabel.getLabelValue()
                           << " , not pop and lookup.";
}

void MPLSHandler::handleUnknownLabel(
    std::unique_ptr<RxPacket> /*pkt*/,
    const MPLSHdr& header,
    folly::io::Cursor /*cursor*/) {
  // unknown mpls label packet dropped.
  // TODO: add ods counter for such dropped mpls packet
  auto topLabel = header.getLookupLabel();
  XLOG_EVERY_MS(ERR, 1000) << "received packet with unknown label:"
                           << topLabel.getLabelValue();
}

void MPLSHandler::handleLabel2Me(
    std::unique_ptr<RxPacket> /*pkt*/,
    const MPLSHdr& /*header*/,
    folly::io::Cursor /*cursor*/) const {
  // TODO: handle labels punted to cpu
}

void MPLSHandler::popLabelAndLookup(
    std::unique_ptr<RxPacket> pkt,
    const MPLSHdr& header,
    folly::io::Cursor cursor) {
  auto topLabel = header.getLookupLabel();
  if (!topLabel.isbottomOfStack()) {
    const auto& stack = header.stack();
    return handlePacket(
        std::move(pkt),
        MPLSHdr(std::vector<MPLSHdr::Label>(stack.begin() + 1, stack.end())),
        cursor);
  }
  auto protocol = utility::decapsulateMplsPacket(pkt->buf());
  if (!protocol) {
    XLOG_EVERY_MS(ERR, 1000)
        << "MPLS packet did not contain v4 or v6 IP packet";
    return;
  }
  // schedule v4 or v6 packet to be handled as usual
  sw_->packetReceived(std::move(pkt));
}

bool MPLSHandler::isLabelProgrammed(const MPLSHdr& header) const {
  auto topLabel = header.getLookupLabel();

  auto labels = sw_->getState()->getMultiLabelForwardingInformationBase();
  if (!labels) {
    return false;
  }
  return labels->getNodeIf(topLabel.getLabelValue()) != nullptr;
}
} // namespace facebook::fboss
