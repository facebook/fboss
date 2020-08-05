// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwNeighborTests.h"

namespace facebook::fboss {

TYPED_TEST_SUITE(HwNeighborTest, NeighborTypes);

TYPED_TEST(HwNeighborTest, AddPendingEntry) {
  auto setup = [this]() {
    auto newState = this->addNeighbor(this->getProgrammedState());
    this->applyNewState(newState);
  };
  auto verify = [this]() {
    EXPECT_TRUE(this->isProgrammedToCPU());
    this->verifyClassId(0);
  };
  if (TypeParam::isTrunk) {
    setup();
    verify();
  } else {
    this->verifyAcrossWarmBoots(setup, verify);
  }
}

TYPED_TEST(HwNeighborTest, ResolvePendingEntry) {
  auto setup = [this]() {
    auto state = this->addNeighbor(this->getProgrammedState());
    auto newState = this->resolveNeighbor(state);
    this->applyNewState(newState);
  };
  auto verify = [this]() {
    EXPECT_FALSE(this->isProgrammedToCPU());
    this->verifyClassId(static_cast<int>(this->kLookupClass));
  };
  if (TypeParam::isTrunk) {
    setup();
    verify();
  } else {
    this->verifyAcrossWarmBoots(setup, verify);
  }
}

TYPED_TEST(HwNeighborTest, UnresolveResolvedEntry) {
  auto setup = [this]() {
    auto state =
        this->resolveNeighbor(this->addNeighbor(this->getProgrammedState()));
    auto newState = this->unresolveNeighbor(state);
    this->applyNewState(newState);
  };
  auto verify = [this]() {
    EXPECT_TRUE(this->isProgrammedToCPU());
    this->verifyClassId(0);
  };
  if (TypeParam::isTrunk) {
    setup();
    verify();
  } else {
    this->verifyAcrossWarmBoots(setup, verify);
  }
}

TYPED_TEST(HwNeighborTest, ResolveThenUnresolveEntry) {
  auto setup = [this]() {
    auto state =
        this->resolveNeighbor(this->addNeighbor(this->getProgrammedState()));
    this->applyNewState(state);
    auto newState = this->unresolveNeighbor(this->getProgrammedState());
    this->applyNewState(newState);
  };
  auto verify = [this]() {
    EXPECT_TRUE(this->isProgrammedToCPU());
    this->verifyClassId(0);
  };
  if (TypeParam::isTrunk) {
    setup();
    verify();
  } else {
    this->verifyAcrossWarmBoots(setup, verify);
  }
}
TYPED_TEST(HwNeighborTest, RemoveResolvedEntry) {
  auto setup = [this]() {
    auto state =
        this->resolveNeighbor(this->addNeighbor(this->getProgrammedState()));
    auto newState = this->removeNeighbor(state);
    this->applyNewState(newState);
  };
  auto verify = [this]() { EXPECT_ANY_THROW(this->isProgrammedToCPU()); };
  if (TypeParam::isTrunk) {
    setup();
    verify();
  } else {
    this->verifyAcrossWarmBoots(setup, verify);
  }
}

TYPED_TEST(HwNeighborTest, AddPendingRemovedEntry) {
  auto setup = [this]() {
    auto state =
        this->resolveNeighbor(this->addNeighbor(this->getProgrammedState()));
    this->applyNewState(this->removeNeighbor(state));
    this->applyNewState(this->addNeighbor(this->getProgrammedState()));
  };
  auto verify = [this]() {
    EXPECT_TRUE(this->isProgrammedToCPU());
    this->verifyClassId(0);
  };
  if (TypeParam::isTrunk) {
    setup();
    verify();
  } else {
    this->verifyAcrossWarmBoots(setup, verify);
  }
}

TYPED_TEST(HwNeighborTest, LinkDownOnResolvedEntry) {
  auto setup = [this]() {
    auto state = this->addNeighbor(this->getProgrammedState());
    auto newState = this->resolveNeighbor(state);
    newState = this->applyNewState(newState);
    this->bringDownPort(this->masterLogicalPortIds()[0]);
  };
  auto verify = [this]() {
    // egress to neighbor entry is not updated on link down
    // if it is not part of ecmp group
    EXPECT_FALSE(this->isProgrammedToCPU());
    this->verifyClassId(static_cast<int>(this->kLookupClass));
  };
  if (TypeParam::isTrunk) {
    setup();
    verify();
  } else {
    this->verifyAcrossWarmBoots(setup, verify);
  }
}

TYPED_TEST(HwNeighborTest, BothLinkDownOnResolvedEntry) {
  auto setup = [this]() {
    auto state = this->addNeighbor(this->getProgrammedState());
    auto newState = this->resolveNeighbor(state);
    newState = this->applyNewState(newState);

    this->bringDownPorts(
        {this->masterLogicalPortIds()[0], this->masterLogicalPortIds()[1]});
  };
  auto verify = [this]() {
    // egress to neighbor entry is not updated on link down
    // if it is not part of ecmp group
    EXPECT_FALSE(this->isProgrammedToCPU());
    this->verifyClassId(static_cast<int>(this->kLookupClass));
  };

  if (TypeParam::isTrunk) {
    setup();
    verify();
  } else {
    this->verifyAcrossWarmBoots(setup, verify);
  }
}

TYPED_TEST(HwNeighborTest, LinkDownAndUpOnResolvedEntry) {
  auto setup = [this]() {
    auto state = this->addNeighbor(this->getProgrammedState());
    auto newState = this->resolveNeighbor(state);
    newState = this->applyNewState(newState);
    this->bringDownPort(this->masterLogicalPortIds()[0]);
    this->bringUpPort(this->masterLogicalPortIds()[0]);
  };
  auto verifyForPort = [this]() {
    EXPECT_FALSE(this->isProgrammedToCPU());
    this->verifyClassId(static_cast<int>(this->kLookupClass));
  };
  auto verifyForTrunk = [this]() { EXPECT_FALSE(this->isProgrammedToCPU()); };

  if (TypeParam::isTrunk) {
    setup();
    verifyForTrunk();
  } else {
    this->verifyAcrossWarmBoots(setup, verifyForPort);
  }
}

} // namespace facebook::fboss
