#include "fboss/agent/TxPacket.h"
#include "fboss/agent/TxPacketObserver.h"

namespace facebook::fboss {

std::unique_ptr<TxPacket> TxPacket::allocateTxPacket(size_t size) {
  incrementPacketCounter();
  return std::unique_ptr<TxPacket>(new TxPacket(size));
}

TxPacket::TxPacket(size_t size) {
  buf_ = folly::IOBuf::create(size);
  buf_->append(size);
  buf_->appendSharedInfoObserver(TxPacketObserver());
}

std::atomic<size_t>* TxPacket::getPacketCounter() {
  static std::atomic<size_t> packetCounter{0};
  return &packetCounter;
}

void TxPacket::decrementPacketCounter() {
  auto packetCounter = getPacketCounter();
  packetCounter->fetch_sub(1, std::memory_order_relaxed);
}
void TxPacket::incrementPacketCounter() {
  auto packetCounter = getPacketCounter();
  packetCounter->fetch_add(1, std::memory_order_relaxed);
}

} // namespace facebook::fboss
