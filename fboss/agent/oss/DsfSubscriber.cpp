// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/DsfSubscriber.h"

namespace facebook::fboss {

std::vector<std::string> DsfSubscriber::getSystemPortsPath() {
  return {"agent", "switchState", "systemPortMaps"};
}

std::vector<std::string> DsfSubscriber::getInterfacesPath() {
  return {"agent", "switchState", "interfaceMaps"};
}
} // namespace facebook::fboss
