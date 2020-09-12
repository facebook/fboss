/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/lib/usb/CP2112.h"
#include "fboss/lib/usb/PCA9548MuxedBus.h"

#include <folly/container/Enumerate.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace facebook::fboss;
using ::testing::_;
using ::testing::InSequence;

class MockCP2112 : public CP2112Intf {
 public:
  MOCK_METHOD1(open, void(bool));
  MOCK_METHOD0(close, void());
  MOCK_METHOD0(resetDevice, void());

  MOCK_METHOD3(
      read,
      void(uint8_t, folly::MutableByteRange, std::chrono::milliseconds));
  using CP2112Intf::read;

  MOCK_METHOD3(
      write,
      void(uint8_t, folly::ByteRange, std::chrono::milliseconds));
  using CP2112Intf::write;

  MOCK_METHOD4(
      writeReadUnsafe,
      void(
          uint8_t,
          folly::ByteRange,
          folly::MutableByteRange,
          std::chrono::milliseconds));
  using CP2112Intf::writeReadUnsafe;

  std::chrono::milliseconds getDefaultTimeout() const override {
    return std::chrono::milliseconds(500);
  }

  bool isOpen() const override {
    return true;
  }
};

int constexpr pow(int base, int exponent) {
  return exponent == 0 ? 1 : base * pow(base, exponent - 1);
}

/*
 * This class is a helper to generate a fully populated mux bus w/
 * NUM_LAYERS layers of muxes and MUXES_PER_LAYER muxes on each layer.
 */
template <int LAYERS, int MUXES_PER_LAYER>
class FakeMuxBus
    : public PCA9548MuxedBus<pow(MUXES_PER_LAYER* PCA9548::WIDTH, LAYERS)> {
 public:
  FakeMuxBus()
      : PCA9548MuxedBus<pow(MUXES_PER_LAYER* PCA9548::WIDTH, LAYERS)>(
            std::make_unique<MockCP2112>()) {}
  MuxLayer createMuxes() override {
    MuxLayer roots;
    for (int i = 0; i < MUXES_PER_LAYER; ++i) {
      roots.push_back(std::make_unique<QsfpMux>(this->dev_.get(), i));
    }

    populateMuxes(roots, 1);

    return roots;
  }

  void wireUpPorts(typename PCA9548MuxedBus<
                   pow(MUXES_PER_LAYER* PCA9548::WIDTH, LAYERS)>::PortLeaves&
                       leaves) override {
    for (const auto&& mux : folly::enumerate(leafMuxes_)) {
      auto start = mux.index * PCA9548::WIDTH;
      this->connectPortsToMux(leaves, *mux, start);
    }
  }

  MuxLayer& roots() {
    return this->roots_;
  }

  void selectQsfp(unsigned int module) {
    this->selectQsfpImpl(module);
  }

  MockCP2112* fakeDev() {
    return static_cast<MockCP2112*>(this->dev_.get());
  }

 private:
  void populateMuxes(MuxLayer& parentLayer, uint8_t currLayer) {
    for (int i = 0; i < parentLayer.size(); ++i) {
      auto& mux = parentLayer[i];

      if (currLayer == LAYERS) {
        leafMuxes_.push_back(mux.get());
        continue;
      }

      for (uint8_t channel = 0; channel < PCA9548::WIDTH; ++channel) {
        for (int k = 0; k < MUXES_PER_LAYER; ++k) {
          auto addr = currLayer * 10 + k;
          mux->registerChildMux(this->dev_.get(), channel, addr);
        }
        auto& nextLayer = mux->children(channel);
        populateMuxes(nextLayer, currLayer + 1);
      }
    }
  }

  // helper just to wire up ports w/out walking tree again
  std::vector<QsfpMux*> leafMuxes_;
};

TEST(PCA9548MuxedBusTests, SingleMux) {
  FakeMuxBus<1, 1> bus;
  bus.open();

  {
    InSequence dummy;

    EXPECT_CALL(*bus.fakeDev(), write(_, _, _)).Times(1);

    // select first module
    bus.selectQsfp(1);
    EXPECT_TRUE(bus.roots()[0]->mux()->isSelected(0));

    EXPECT_CALL(*bus.fakeDev(), write(_, _, _)).Times(1);

    // unselect, nothing should be selected
    bus.selectQsfp(0);
    EXPECT_EQ(bus.roots()[0]->mux()->selected(), 0);
  }
}

TEST(PCA9548MuxedBusTests, SingleLayerMultipleMuxes) {
  FakeMuxBus<1, 4> bus;
  bus.open();

  {
    InSequence dummy;

    // select first module
    EXPECT_CALL(*bus.fakeDev(), write(_, _, _)).Times(1);

    bus.selectQsfp(1);
    EXPECT_TRUE(bus.roots()[0]->mux()->isSelected(0));
    EXPECT_EQ(bus.roots()[1]->mux()->selected(), 0);
    EXPECT_EQ(bus.roots()[2]->mux()->selected(), 0);
    EXPECT_EQ(bus.roots()[3]->mux()->selected(), 0);

    // second mux, second channel, need one write to unselect first
    // mux, one to select second

    EXPECT_CALL(*bus.fakeDev(), write(_, _, _)).Times(2);

    bus.selectQsfp(10);
    EXPECT_TRUE(bus.roots()[1]->mux()->isSelected(1));
    EXPECT_EQ(bus.roots()[0]->mux()->selected(), 0);
    EXPECT_EQ(bus.roots()[2]->mux()->selected(), 0);
    EXPECT_EQ(bus.roots()[3]->mux()->selected(), 0);

    // second mux, third channel. Only one write to second mux needed

    EXPECT_CALL(*bus.fakeDev(), write(_, _, _)).Times(1);

    bus.selectQsfp(11);
    EXPECT_TRUE(bus.roots()[1]->mux()->isSelected(2));
    EXPECT_EQ(bus.roots()[0]->mux()->selected(), 0);
    EXPECT_EQ(bus.roots()[2]->mux()->selected(), 0);
    EXPECT_EQ(bus.roots()[3]->mux()->selected(), 0);

    // unselect, nothing should be selected
    EXPECT_CALL(*bus.fakeDev(), write(_, _, _)).Times(1);

    bus.selectQsfp(0);
    EXPECT_EQ(bus.roots()[0]->mux()->selected(), 0);
    EXPECT_EQ(bus.roots()[1]->mux()->selected(), 0);
    EXPECT_EQ(bus.roots()[2]->mux()->selected(), 0);
    EXPECT_EQ(bus.roots()[3]->mux()->selected(), 0);
  }
}

TEST(PCA9548MuxedBusTests, MultiLayerSingleMux) {
  FakeMuxBus<3, 1> bus;
  bus.open();

  auto& root = bus.roots()[0];

  {
    InSequence dummy;

    // select first module, need to write to three muxes
    EXPECT_CALL(*bus.fakeDev(), write(_, _, _)).Times(3);

    bus.selectQsfp(1);
    EXPECT_TRUE(root->mux()->isSelected(0));
    EXPECT_TRUE(root->children(0)[0]->mux()->isSelected(0));
    EXPECT_TRUE(root->children(0)[0]->children(0)[0]->mux()->isSelected(0));

    // select last module, should do 5 writes, two unselections, three
    // selections
    EXPECT_CALL(*bus.fakeDev(), write(_, _, _)).Times(5);

    bus.selectQsfp(512);
    EXPECT_FALSE(root->mux()->isSelected(0));
    EXPECT_FALSE(root->children(0)[0]->mux()->isSelected(0));
    EXPECT_FALSE(root->children(0)[0]->children(0)[0]->mux()->isSelected(0));
    EXPECT_TRUE(root->mux()->isSelected(7));
    EXPECT_TRUE(root->children(7)[0]->mux()->isSelected(7));
    EXPECT_TRUE(root->children(7)[0]->children(7)[0]->mux()->isSelected(7));

    // select last module - 1. Should do only one write.
    EXPECT_CALL(*bus.fakeDev(), write(_, _, _)).Times(1);

    bus.selectQsfp(511);
    EXPECT_TRUE(root->mux()->isSelected(7));
    EXPECT_TRUE(root->children(7)[0]->mux()->isSelected(7));
    EXPECT_FALSE(root->children(7)[0]->children(7)[0]->mux()->isSelected(7));
    EXPECT_TRUE(root->children(7)[0]->children(7)[0]->mux()->isSelected(6));

    // select same module again. Should do zero writes
    EXPECT_CALL(*bus.fakeDev(), write(_, _, _)).Times(0);

    bus.selectQsfp(511);
    EXPECT_TRUE(root->mux()->isSelected(7));
    EXPECT_TRUE(root->children(7)[0]->mux()->isSelected(7));
    EXPECT_FALSE(root->children(7)[0]->children(7)[0]->mux()->isSelected(7));
    EXPECT_TRUE(root->children(7)[0]->children(7)[0]->mux()->isSelected(6));

    // unselect, nothing should be selected
    EXPECT_CALL(*bus.fakeDev(), write(_, _, _)).Times(3);

    bus.selectQsfp(0);
    EXPECT_EQ(root->mux()->selected(), 0);
    EXPECT_EQ(root->children(7)[0]->mux()->selected(), 0);
    EXPECT_EQ(root->children(7)[0]->children(7)[0]->mux()->selected(), 0);
  }
}

TEST(PCA9548MuxedBusTests, MultiLayerMultiMux) {
  FakeMuxBus<2, 2> bus;
  bus.open();

  auto& root1 = bus.roots()[0];
  auto& root2 = bus.roots()[1];

  {
    InSequence dummy;

    // select first module, need to write to two muxes
    EXPECT_CALL(*bus.fakeDev(), write(_, _, _)).Times(2);

    bus.selectQsfp(1);
    EXPECT_TRUE(root1->mux()->isSelected(0));
    EXPECT_TRUE(root1->children(0)[0]->mux()->isSelected(0));

    // select module 9 (still in same second layer, but different
    // mux). We need two writes.
    EXPECT_CALL(*bus.fakeDev(), write(_, _, _)).Times(2);

    bus.selectQsfp(9);
    EXPECT_TRUE(root1->mux()->isSelected(0));
    EXPECT_FALSE(root1->children(0)[0]->mux()->isSelected(0));
    EXPECT_TRUE(root1->children(0)[1]->mux()->isSelected(0));

    // select last module, should do 4 writes, two unselections, two
    // selections
    EXPECT_CALL(*bus.fakeDev(), write(_, _, _)).Times(4);

    bus.selectQsfp(256);
    EXPECT_FALSE(root1->mux()->isSelected(0));
    EXPECT_FALSE(root1->children(0)[1]->mux()->isSelected(0));
    EXPECT_TRUE(root2->mux()->isSelected(7));
    EXPECT_TRUE(root2->children(7)[1]->mux()->isSelected(7));

    // select last module - 1. Should do only one write since it is a
    // different channel on the same mux.
    EXPECT_CALL(*bus.fakeDev(), write(_, _, _)).Times(1);

    bus.selectQsfp(255);
    EXPECT_TRUE(root2->mux()->isSelected(7));
    EXPECT_FALSE(root2->children(7)[1]->mux()->isSelected(7));
    EXPECT_TRUE(root2->children(7)[1]->mux()->isSelected(6));

    // select same module again. Should do zero writes
    EXPECT_CALL(*bus.fakeDev(), write(_, _, _)).Times(0);

    bus.selectQsfp(255);
    EXPECT_TRUE(root2->mux()->isSelected(7));
    EXPECT_FALSE(root2->children(7)[1]->mux()->isSelected(7));
    EXPECT_TRUE(root2->children(7)[1]->mux()->isSelected(6));

    // unselect everything and ensure nothing is selected
    EXPECT_CALL(*bus.fakeDev(), write(_, _, _)).Times(2);

    bus.selectQsfp(0);
    EXPECT_EQ(root2->mux()->selected(), 0);
    EXPECT_EQ(root2->children(7)[1]->mux()->selected(), 0);
    EXPECT_EQ(root2->children(7)[1]->mux()->selected(), 0);
  }
}
