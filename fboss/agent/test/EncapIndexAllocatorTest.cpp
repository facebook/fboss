/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/EncapIndexAllocator.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/switch_asics/MockAsic.h"
#include "fboss/agent/hw/switch_asics/TomahawkAsic.h"
#include "fboss/agent/state/NeighborEntry.h"
#include "fboss/agent/state/StateUtils.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

class EncapIndexAllocatorTest : public ::testing::Test {
 public:
  void SetUp() override {
    auto config = testConfigA(cfg::SwitchType::VOQ);
    handle = createTestHandle(&config);
  }
  SwSwitch* getSw() const {
    return handle->getSw();
  }
  const HwAsic& getAsic() const {
    return *(handle->getPlatform()->getAsic());
  }

  int64_t allocationStart() const {
    return *getAsic().getReservedEncapIndexRange().minimum() +
        EncapIndexAllocator::kEncapIdxReservedForLoopbacks;
  }

  std::shared_ptr<SwitchState> addNeighborToInterface(
      std::shared_ptr<SwitchState> state,
      const folly::IPAddressV6& ip,
      int64_t encapIdx) const {
    const auto& intfMap = state->getInterfaces()->cbegin()->second;
    const auto& firstIntf = intfMap->cbegin()->second;
    state::NeighborEntryFields nbr;
    nbr.mac() = "02:00:00:00:00:01";
    nbr.interfaceId() = static_cast<int>(firstIntf->getID());
    nbr.ipaddress() = ip.str();
    nbr.portId() =
        PortDescriptor(utility::getFirstMap(getSw()->getState()->getPorts())
                           ->cbegin()
                           ->second->getID())
            .toThrift();
    nbr.state() = state::NeighborState::Reachable;
    nbr.encapIndex() = encapIdx;
    auto nbrTable = firstIntf->getNdpTable()->toThrift();
    nbrTable.insert({*nbr.ipaddress(), nbr});
    auto interface = firstIntf->modify(&state);
    interface->setNdpTable(nbrTable);
    return state;
  }

  std::unique_ptr<HwTestHandle> handle;
  EncapIndexAllocator allocator;
};

TEST_F(EncapIndexAllocatorTest, unsupportedAsic) {
  cfg::SwitchInfo swInfo;
  swInfo.switchType() = cfg::SwitchType::NPU;
  swInfo.switchIndex() = 0;
  swInfo.switchMac() = folly::MacAddress().toString();
  auto asic = std::make_unique<TomahawkAsic>(0, swInfo);
  EXPECT_THROW(allocator.getNextAvailableEncapIdx(nullptr, *asic), FbossError);
}

TEST_F(EncapIndexAllocatorTest, firstAllocation) {
  EXPECT_EQ(
      allocator.getNextAvailableEncapIdx(getSw()->getState(), getAsic()),
      allocationStart());
}

TEST_F(EncapIndexAllocatorTest, intfNeighbors) {
  auto stateV1 = addNeighborToInterface(
      getSw()->getState(),
      folly::IPAddressV6{"2401:db00:2110:3001::0002"},
      allocationStart());
  // Next encap idx allocation
  EXPECT_EQ(
      allocator.getNextAvailableEncapIdx(stateV1, getAsic()),
      allocationStart() + 1);
  // Add a neighbor while creating a hole
  auto stateV2 = addNeighborToInterface(
      stateV1,
      folly::IPAddressV6{"2401:db00:2110:3001::0004"},
      allocationStart() + 2);
  EXPECT_EQ(
      allocator.getNextAvailableEncapIdx(stateV2, getAsic()),
      allocationStart() + 1);
  // Fill the hole, next one should be after all the contiguously allocated
  // indices
  auto stateV3 = addNeighborToInterface(
      stateV2,
      folly::IPAddressV6{"2401:db00:2110:3001::0003"},
      allocationStart() + 1);
  EXPECT_EQ(
      allocator.getNextAvailableEncapIdx(stateV3, getAsic()),
      allocationStart() + 3);
}
