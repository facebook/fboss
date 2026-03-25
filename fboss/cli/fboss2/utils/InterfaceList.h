/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <string>
#include <utility>
#include <vector>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"

namespace facebook::fboss::utils {

/*
 * Intf represents a unified interface/port object that can contain
 * a pointer to a cfg::Port, a cfg::Interface, or both.
 */
class Intf {
 public:
  explicit Intf(std::string name)
      : name_(std::move(name)), port_(nullptr), interface_(nullptr) {}

  /* Get the Port pointer (may be nullptr). */
  cfg::Port* getPort() const {
    return port_;
  }

  /* Get the Interface pointer (may be nullptr). */
  cfg::Interface* getInterface() const {
    return interface_;
  }

  /* Set the Port pointer. */
  void setPort(cfg::Port* port) {
    port_ = port;
  }

  /* Set the Interface pointer. */
  void setInterface(cfg::Interface* interface) {
    interface_ = interface;
  }

  /* Get the name of this interface. */
  const std::string& name() const {
    return name_;
  }

  /* Check if this Intf has either a Port or Interface. */
  bool isValid() const {
    return port_ != nullptr || interface_ != nullptr;
  }

 private:
  std::string name_;
  cfg::Port* port_;
  cfg::Interface* interface_;
};

/*
 * InterfaceList resolves port/interface names to Intf objects.
 * For each name, it looks up both the port and the interface.
 * First tries to look up as a port name, then as an interface name.
 */
class InterfaceList : public BaseObjectArgType<Intf> {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ InterfaceList(std::vector<std::string> names);

  /* Get the original names provided by the user. */
  const std::vector<std::string>& getNames() const {
    return names_;
  }

  const static ObjectArgTypeId id =
      ObjectArgTypeId::OBJECT_ARG_TYPE_ID_INTERFACE_LIST;

 private:
  std::vector<std::string> names_;
};

} // namespace facebook::fboss::utils
