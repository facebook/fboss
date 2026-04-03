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
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"

namespace facebook::fboss {

/*
 * InterfacesConfig captures both port/interface names and optional
 * attribute key-value pairs from the CLI.
 *
 * Usage: config interface <port-list> [<attr> <value> ...]
 *
 * The first tokens (until a known attribute name is encountered) are
 * treated as port/interface names. The remaining tokens are parsed
 * as attribute-value pairs.
 *
 * Supported attributes: description, mtu
 */
class InterfacesConfig : public utils::BaseObjectArgType<std::string> {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ InterfacesConfig(std::vector<std::string> v);

  /* Get the resolved interfaces. */
  const utils::InterfaceList& getInterfaces() const {
    return interfaces_;
  }

  /* Implicit conversion to InterfaceList so that child commands can accept
   * const InterfaceList& without knowing about InterfacesConfig. */
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ operator const utils::InterfaceList&() const {
    return interfaces_;
  }

  /* Get the attribute-value pairs. */
  const std::vector<std::pair<std::string, std::string>>& getAttributes()
      const {
    return attributes_;
  }

  /* Check if any attributes were provided. */
  bool hasAttributes() const {
    return !attributes_.empty();
  }

  const static utils::ObjectArgTypeId id =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_INTERFACES_CONFIG;

 private:
  utils::InterfaceList interfaces_;
  std::vector<std::pair<std::string, std::string>> attributes_;

  // Check if a string is a known attribute name
  static bool isKnownAttribute(const std::string& s);
};

struct CmdConfigInterfaceTraits : public WriteCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_INTERFACES_CONFIG;
  using ObjectArgType = InterfacesConfig;
  using RetType = std::string;
};

class CmdConfigInterface
    : public CmdHandler<CmdConfigInterface, CmdConfigInterfaceTraits> {
 public:
  using ObjectArgType = CmdConfigInterfaceTraits::ObjectArgType;
  using RetType = CmdConfigInterfaceTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& interfaceConfig);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
