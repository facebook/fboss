// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include <gtest/gtest.h>
#include "folly/IPAddressV6.h"
#include "folly/MacAddress.h"

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/packet/SflowStructs.h"

namespace facebook::fboss {

TEST(SflowPacketTest, NonBlockMultiplePayloadConstructsWithoutException) {
  EXPECT_NO_THROW({
    std::unique_ptr<TxPacket> sFlowPacket{utility::makeSflowV5Packet(
        &TxPacket::allocateTxPacket,
        /*vlan=*/std::nullopt,
        /*srcMac=*/folly::MacAddress("01:00:00:00:00:01"),
        /*dstMac=*/folly::MacAddress("02:00:00:00:00:02"),
        /*srcIp=*/folly::IPAddressV6("1::1"),
        /*dstIp=*/folly::IPAddressV6("2::2"),
        /*srcPort=*/1,
        /*dstPort=*/2,
        /*trafficClass=*/0,
        /*hopLimit=*/64,
        /*ingressInterface=*/0,
        /*egressInterface=*/0,
        /*samplingRate=*/1,
        /*computeChecksum=*/false,
        /*payload=*/
        std::vector<uint8_t>(10 * sflow::XDR_BASIC_BLOCK_SIZE - 1, 0xff))};
  });
}

} // namespace facebook::fboss
