/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/api/HostifApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

#include <vector>

using namespace facebook::fboss;

class HostifApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    hostifApi = std::make_unique<HostifPacketApi>();
  }
  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<HostifPacketApi> hostifApi;
};

TEST_F(HostifApiTest, sendPacket) {
  HostifApiParameters::TxPacketAttributes::EgressPortOrLag egressPort(10);
  HostifApiParameters::TxPacketAttributes::TxType
    txType(SAI_HOSTIF_TX_TYPE_PIPELINE_BYPASS);
  HostifApiParameters::TxPacketAttributes a{{txType, egressPort}};
  folly::StringPiece testPacket = "TESTPACKET";
  HostifApiParameters::HostifApiPacket txPacket{
    (void *)(testPacket.toString().c_str()),
    testPacket.toString().length()};
  hostifApi->send(a.attrs(), 0, txPacket);
}
