// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/packet/MPLSHdr.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/IPv4Hdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"

#include <folly/io/Cursor.h>
#include <sstream>

namespace facebook::fboss {

MPLSHdr::MPLSHdr(folly::io::Cursor* cursor) {
  MPLSLabel mplsLabel;
  while (cursor->tryReadBE<uint32_t>(mplsLabel.u32)) {
    stack_.push_back(mplsLabel.label);
    if (mplsLabel.label.bottomOfStack) {
      break;
    }
  }
}

void MPLSHdr::serialize(folly::io::RWPrivateCursor* cursor) const {
  MPLSLabel mplsLabel;
  for (auto label : stack_) {
    mplsLabel.label = label;
    cursor->writeBE<uint32_t>(mplsLabel.u32);
  }
}

std::string MPLSHdr::Label::toString() const {
  std::stringstream ss;
  ss << " Label: " << getLabelValue() << " TTL: " << getTTL()
     << " bottomOfStack: " << isbottomOfStack()
     << " trafficClass: " << getTrafficClass();
  return ss.str();
}

std::string MPLSHdr::toString() const {
  std::stringstream ss;
  for (const auto& label : stack_) {
    ss << label.toString() << std::endl;
  }
  return ss.str();
}
namespace utility {
std::optional<ETHERTYPE> decapsulateMplsPacket(folly::IOBuf* buf) {
  folly::io::Cursor cursor(buf);
  EthHdr ethHdr(cursor);
  MPLSHdr mplsHdr(&cursor);
  uint8_t version{0};
  cursor.pull(&version, 1);
  version >>= 4;
  if (version != IPV4_VERSION && version != IPV6_VERSION) {
    // process only v4 or v6 encapsulated packets
    return std::nullopt;
  }

  auto etherType = (version == IPV4_VERSION) ? ETHERTYPE::ETHERTYPE_IPV4
                                             : ETHERTYPE::ETHERTYPE_IPV6;
  // source mac & destination
  size_t srcMacDstMacSize = folly::MacAddress::SIZE + folly::MacAddress::SIZE;
  size_t vlanTagsSize = 0;
  if (!ethHdr.getVlanTags().empty()) {
    // vlan tag is 2 byte vlan tag type + 2 byte vlan tag
    vlanTagsSize += (4 * ethHdr.getVlanTags().size());
  }
  size_t etherTypeSize = sizeof(ETHERTYPE);
  size_t mplsHdrSize = mplsHdr.size();

  // mpls payload is everything after label stack
  auto mplsPayload = buf->clone();
  mplsPayload->trimStart(
      srcMacDstMacSize + vlanTagsSize + etherTypeSize + mplsHdrSize);

  auto totalLength = buf->length();

  // chop off every thing after ether type
  buf->trimEnd(totalLength - srcMacDstMacSize - vlanTagsSize - etherTypeSize);

  buf->appendChain(std::move(mplsPayload));
  buf->coalesce();

  folly::io::RWPrivateCursor rwCursor(buf);
  rwCursor += srcMacDstMacSize;
  rwCursor += vlanTagsSize;
  rwCursor.writeBE<uint16_t>(static_cast<uint16_t>(etherType));

  return etherType;
}

} // namespace utility
} // namespace facebook::fboss
