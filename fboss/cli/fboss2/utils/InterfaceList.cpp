/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/utils/InterfaceList.h"
#include <folly/String.h>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/PortMap.h"

namespace facebook::fboss::utils {

InterfaceList::InterfaceList(std::vector<std::string> names)
    : names_(std::move(names)) {
  // Get the PortMap from the session
  auto& portMap = ConfigSession::getInstance().getPortMap();

  // Resolve names to Intf objects
  std::vector<std::string> notFound;

  for (const auto& name : names_) {
    Intf intf(name);

    // First try to look up as a port name
    cfg::Port* port = portMap.getPort(name);
    if (port) {
      intf.setPort(port);
      // Also try to get the associated interface
      auto interfaceId = portMap.getInterfaceIdForPort(name);
      if (interfaceId) {
        cfg::Interface* interface = portMap.getInterface(*interfaceId);
        if (interface) {
          intf.setInterface(interface);
        }
      }
    } else {
      // If not found as a port name, try as an interface name
      cfg::Interface* interface = portMap.getInterfaceByName(name);
      if (interface) {
        intf.setInterface(interface);
      }
    }

    if (!intf.isValid()) {
      notFound.push_back(name);
    } else {
      data_.push_back(intf);
    }
  }

  if (!notFound.empty()) {
    throw std::invalid_argument(
        "Port(s) or interface(s) not found in configuration: " +
        folly::join(", ", notFound) +
        ". Ports must exist in the hardware platform mapping and be defined "
        "in the configuration before they can be configured.");
  }
}

} // namespace facebook::fboss::utils
