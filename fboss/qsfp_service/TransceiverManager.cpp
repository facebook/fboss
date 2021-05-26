// Copyright 2021-present Facebook. All Rights Reserved.
#include "fboss/qsfp_service/TransceiverManager.h"

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/gen-cpp2/agent_config_types.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

namespace facebook {
namespace fboss {
const TransceiverManager::PortNameMap&
TransceiverManager::getPortNameToModuleMap() const {
  if (portNameToModule_.empty()) {
    const auto& platformPorts = platformMapping_->getPlatformPorts();
    for (const auto& it : platformPorts) {
      auto port = it.second;
      auto transceiverId =
          utility::getTransceiverId(port, platformMapping_->getChips());
      if (!transceiverId) {
        continue;
      }

      auto& portName = *(port.mapping_ref()->name_ref());
      portNameToModule_[portName] = transceiverId.value();
    }
  }

  return portNameToModule_;
}
} // namespace fboss
} // namespace facebook
