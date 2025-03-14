#include "fboss/agent/test/utils/MacLearningFloodHelper.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/QosTestUtils.h"

namespace facebook::fboss::utility {

void MacLearningFloodHelper::startClearMacTable() {
  if (done_ == false) {
    throw FbossError("Mac flap already started");
  }
  done_ = false;

  sendMacTestTraffic();

  WITH_RETRIES({
    auto statsBefore = ensemble_->getSw()->getHwPortStats({dstPortID_});
    auto outBytes = *statsBefore[dstPortID_].outBytes_();
    EXPECT_EVENTUALLY_TRUE(outBytes > 0);
  });

  clearMacTableThread_ = std::thread(&MacLearningFloodHelper::macFlap, this);
}

void MacLearningFloodHelper::sendMacTestTraffic() {
  /*
  auto macLastInterface = getInterfaceMac(
      ensemble_->getProgrammedState(),
      ensemble_->getProgrammedState()
          ->getPorts()
          ->getNodeIf(dstPortID_)
          ->getIngressVlan());*/
  // Send packet
  for (auto mac : macs_) {
    char macByteHex[2];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    std::snprintf(macByteHex, sizeof(macByteHex), "%lx", mac.u64HBO() & 0xffff);
    auto txPacket = utility::makeTCPTxPacket(
        ensemble_->getSw(),
        vlan_,
        mac,
        // MacAddress::BROADCAST send from second last interface to last
        // if replaced by macLastInterface, then traffic won't flood
        MacAddress::BROADCAST,
        folly::IPAddressV6("2620:0:1cfe:face:b00c::" + std::string(macByteHex)),
        folly::IPAddressV6("2620:0:1cfe:face:b10c::4"),
        47231,
        277);
    ensemble_->getSw()->sendPacketOutOfPortAsync(
        std::move(txPacket), dstPortID_);
  }
}
#pragma GCC diagnostic pop

void MacLearningFloodHelper::macFlap() {
  XLOG(DBG2) << "MacLearningFloodHelper started thread id: " << gettid();
  while (!done_) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait_for(
        lock, macTableFlushIntervalMs_, [this] { return done_.load(); });
    MacLearningFloodHelper::clearMacTable();
  }
}

void MacLearningFloodHelper::clearMacTable() {
  XLOG(DBG6) << "# of mac: "
             << ensemble_->getProgrammedState()
                    ->getVlans()
                    ->getNodeIf(vlan_)
                    ->getMacTable()
                    ->size();
  ensemble_->applyNewState([&](const std::shared_ptr<SwitchState>& state) {
    auto newState = state->clone();
    Vlan* vlan = newState->getVlans()->getNode(vlan_).get();
    CHECK_NE(vlan, nullptr);
    vlan = vlan->modify(&newState);
    auto newMacTablePtr = std::make_shared<MacTable>();
    vlan->setMacTable(std::move(newMacTablePtr));
    return newState;
  });
}

void MacLearningFloodHelper::stopClearMacTable() {
  done_ = true;
  cv_.notify_one();
  if (clearMacTableThread_.joinable()) {
    clearMacTableThread_.join();
  }
  ensemble_->bringDownPorts({dstPortID_});
  ensemble_->bringUpPorts({dstPortID_});
}
} // namespace facebook::fboss::utility
