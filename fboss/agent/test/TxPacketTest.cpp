#include "fboss/agent/TxPacket.h"
#include <gtest/gtest.h>
#include <cstring>

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

/*
 * Testing that clone produces a packet with identical content
 */
TEST_F(TxPacketTest, CloneProducesIdenticalContent) {
  constexpr size_t kSize = 64;
  auto pkt = TxPacket::allocateTxPacket(kSize);
  auto* data = pkt->buf()->writableData();
  for (size_t i = 0; i < kSize; i++) {
    data[i] = static_cast<uint8_t>(i);
  }
  auto cloned = pkt->clone();
  EXPECT_EQ(cloned->buf()->computeChainDataLength(), kSize);
  EXPECT_EQ(std::memcmp(pkt->buf()->data(), cloned->buf()->data(), kSize), 0);
}

/*
 * Testing that clone produces an independent buffer
 */
TEST_F(TxPacketTest, CloneIsIndependent) {
  constexpr size_t kSize = 32;
  auto pkt = TxPacket::allocateTxPacket(kSize);
  std::memset(pkt->buf()->writableData(), 0xAA, kSize);
  auto cloned = pkt->clone();
  std::memset(cloned->buf()->writableData(), 0xBB, kSize);
  // Original must be unchanged
  EXPECT_EQ(pkt->buf()->data()[0], 0xAA);
  EXPECT_EQ(cloned->buf()->data()[0], 0xBB);
}

/*
 * Testing that clone increments the packet counter
 */
TEST_F(TxPacketTest, CloneIncrementsPacketCounter) {
  auto initialCount = TxPacket::getPacketCounter()->load();
  auto pkt = TxPacket::allocateTxPacket(16);
  EXPECT_EQ(TxPacket::getPacketCounter()->load(), initialCount + 1);
  auto cloned = pkt->clone();
  EXPECT_EQ(TxPacket::getPacketCounter()->load(), initialCount + 2);
}

} // namespace facebook::fboss
