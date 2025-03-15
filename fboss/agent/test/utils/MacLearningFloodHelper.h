#pragma once
#include "fboss/agent/test/AgentEnsemble.h"

namespace {
const int kNumOfMacs = 1024;
}

DECLARE_bool(intf_nbr_tables);

namespace facebook::fboss::utility {
class MacLearningFloodHelper {
 public:
  MacLearningFloodHelper(
      AgentEnsemble* ensemble,
      PortID srcPortID,
      PortID dstPortID,
      VlanID vlan,
      int macTableFlushIntervalMs = 1000) {
    ensemble_ = ensemble;
    srcPortID_ = srcPortID;
    dstPortID_ = dstPortID;
    macTableFlushIntervalMs_ =
        std::chrono::milliseconds(macTableFlushIntervalMs);
    vlan_ = vlan;
    for (int i = 0; i < kNumOfMacs; i++) {
      // randomly generate mac address
      uint64_t bytes = 0xFEEEC2000000;
      macs_.push_back(folly::MacAddress::fromHBO(bytes + i));
    }
  }
  ~MacLearningFloodHelper() {
    macs_.clear();
    if (done_) {
      return;
    }
    stopClearMacTable();
  }
  void startClearMacTable();

  void stopClearMacTable();

 protected:
  void macFlap();
  void clearMacTable();
  void sendMacTestTraffic();
  AgentEnsemble* ensemble_;
  PortID srcPortID_;
  PortID dstPortID_;
  VlanID vlan_;
  std::vector<folly::MacAddress> macs_;
  std::thread clearMacTableThread_;
  std::atomic<bool> done_ = true;
  std::condition_variable cv_;
  std::chrono::milliseconds macTableFlushIntervalMs_;
  std::mutex mutex_;
};
} // namespace facebook::fboss::utility
