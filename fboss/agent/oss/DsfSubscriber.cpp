// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/DsfSubscriber.h"

namespace facebook::fboss {

std::vector<std::string> DsfSubscriber::getSystemPortsPath() {
  return {"agent", "switchState", "systemPortMaps"};
}

std::vector<std::string> DsfSubscriber::getInterfacesPath() {
  return {"agent", "switchState", "interfaceMaps"};
}

std::vector<std::string> DsfSubscriber::getDsfSubscriptionsPath(
    const std::string& localNodeName) {
  return {"dsfSubscriptions", localNodeName};
}

} // namespace facebook::fboss
