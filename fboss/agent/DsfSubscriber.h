// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/StateObserver.h"

#include <gtest/gtest.h>
#include <memory>

DECLARE_bool(dsf_subscriber_skip_hw_writes);
DECLARE_bool(dsf_subscriber_cache_updated_state);

namespace facebook::fboss {
class SwSwitch;
class SwitchState;
class InterfaceMap;
class SystemPortMap;
namespace fsdb {
class FsdbPubSubManager;
}

class DsfSubscriber : public StateObserver {
 public:
  explicit DsfSubscriber(SwSwitch* sw);
  ~DsfSubscriber() override;
  void stateUpdated(const StateDelta& stateDelta) override;

  void stop();

  const std::shared_ptr<SwitchState> cachedState() const {
    return cachedState_;
  }

 private:
  void scheduleUpdate(
      const std::shared_ptr<SystemPortMap>& newSysPorts,
      const std::shared_ptr<InterfaceMap>& newRifs,
      const std::string& nodeName,
      SwitchID nodeSwitchId);
  // Paths
  static std::vector<std::string> getSystemPortsPath();
  static std::vector<std::string> getInterfacesPath();
  SwSwitch* sw_;
  std::unique_ptr<fsdb::FsdbPubSubManager> fsdbPubSubMgr_;
  std::shared_ptr<SwitchState> cachedState_;
  FRIEND_TEST(DsfSubscriberTest, scheduleUpdate);
  FRIEND_TEST(DsfSubscriberTest, setupNeighbors);
};

} // namespace facebook::fboss
