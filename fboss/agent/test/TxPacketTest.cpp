#include "fboss/agent/TxPacket.h"
#include <gtest/gtest.h>

namespace facebook::fboss {

class PacketOwner {
 public:
  void takeOwnership(std::unique_ptr<TxPacket> packet) {
    ownedPacket_ = std::move(packet);
  }

  void destroyOwnedPacket() {
    ownedPacket_.reset();
  }

  const TxPacket* getOwnedPacket() const {
    return ownedPacket_.get();
  }

 private:
  std::unique_ptr<TxPacket> ownedPacket_;
};

class TxPacketTest : public ::testing::Test {
 protected:
  void SetUp() override {}
};

/*
 * Testing that counter increments when a new tx packet is allocated
 */
TEST_F(TxPacketTest, AllocateTxPacketIncreasesCounter) {
  auto initialCount = TxPacket::getPacketCounter()->load();
  auto packet = TxPacket::allocateTxPacket(128);
  EXPECT_EQ(TxPacket::getPacketCounter()->load(), initialCount + 1);
}

/*
 * Testing that transferring ownership doesn't increase counter
 */
TEST_F(TxPacketTest, AllocateAndTransferOwnership) {
  auto initialCount = TxPacket::getPacketCounter()->load();
  auto packet = TxPacket::allocateTxPacket(128);

  PacketOwner owner;
  owner.takeOwnership(std::move(packet));

  EXPECT_EQ(TxPacket::getPacketCounter()->load(), initialCount + 1);
  EXPECT_EQ(owner.getOwnedPacket() != nullptr, true);
  EXPECT_EQ(
      packet == nullptr, true); // Ensure original pointer is null after move
}

/*
 * Testing that counter decrements when packet is destroyed
 */
TEST_F(TxPacketTest, DestroyOwnedPacket) {
  auto initialCount = TxPacket::getPacketCounter()->load();
  auto packet = TxPacket::allocateTxPacket(128);

  PacketOwner owner;
  owner.takeOwnership(std::move(packet));
  owner.destroyOwnedPacket();

  EXPECT_EQ(TxPacket::getPacketCounter()->load(), initialCount);
  EXPECT_EQ(owner.getOwnedPacket() == nullptr, true);
}

} // namespace facebook::fboss
