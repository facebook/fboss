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

#include "fboss/cli/fboss2/CmdHandler.h"

namespace facebook::fboss {

// Argument type for interface configuration deletion
// Parses: <interface> <attribute> <value>
// where attribute is: ip-address or ipv6-address
//
// Note: Unlike config interface, this only accepts ONE interface name.
// This is because an IP address can only be configured on one interface
// within a VRF, so deleting from multiple interfaces doesn't make sense.
class InterfaceDeleteConfig : public utils::BaseObjectArgType<std::string> {
 public:
  /* implicit */ InterfaceDeleteConfig( // NOLINT(google-explicit-constructor)
      std::vector<std::string> v);

  const std::string& getInterfaceName() const {
    return interfaceName_;
  }

  const std::string& getAttribute() const {
    return attribute_;
  }

  const std::string& getValue() const {
    return value_;
  }

  const static utils::ObjectArgTypeId id =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_INTERFACE_DELETE_CONFIG;

 private:
  std::string interfaceName_;
  std::string attribute_;
  std::string value_;

  static bool isKnownAttribute(const std::string& s);
};

// Delete interface traits
struct CmdDeleteInterfaceTraits : public WriteCommandTraits {
  using ParentCmd = void;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_INTERFACE_DELETE_CONFIG;
  using ObjectArgType = InterfaceDeleteConfig;
  using RetType = std::string;
};

class CmdDeleteInterface
    : public CmdHandler<CmdDeleteInterface, CmdDeleteInterfaceTraits> {
 public:
  using ObjectArgType = CmdDeleteInterfaceTraits::ObjectArgType;
  using RetType = CmdDeleteInterfaceTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& deleteConfig);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
