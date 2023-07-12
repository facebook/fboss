// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/utils/TeFlowUtils.h"
#include "fboss/agent/AddressUtil.h"

namespace facebook::fboss {

IpPrefix ipPrefix(const folly::StringPiece ip, int length) {
  IpPrefix result;
  result.ip() = facebook::network::toBinaryAddress(folly::IPAddress(ip));
  result.prefixLength() = length;
  return result;
}

TeFlow
makeFlowKey(const std::string& dstIp, int prefixLength, uint16_t srcPort) {
  TeFlow flow;
  flow.srcPort() = srcPort;
  flow.dstPrefix() = ipPrefix(dstIp, prefixLength);
  return flow;
}

FlowEntry makeFlow(
    const std::string& dstIp,
    const std::string& nhop,
    const std::string& ifname,
    const uint16_t& srcPort,
    const std::string& counterID,
    const int& prefixLength) {
  FlowEntry flowEntry;
  flowEntry.flow()->srcPort() = srcPort;
  flowEntry.flow()->dstPrefix() = ipPrefix(dstIp, prefixLength);
  flowEntry.counterID() = counterID;
  flowEntry.nextHops()->resize(1);
  flowEntry.nextHops()[0].address() =
      facebook::network::toBinaryAddress(folly::IPAddress(nhop));
  flowEntry.nextHops()[0].address()->ifName() = ifname;
  return flowEntry;
}
} // namespace facebook::fboss
