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
#include <vector>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"

namespace facebook::fboss {

/*
 * InterfaceDeleteAttrs captures both port/interface names and optional
 * attribute names (valueless) from the CLI for the delete interface command.
 *
 * Usage: delete interface <port-list> [<attr> ...]
 *
 * The first tokens (until a known attribute name is encountered) are
 * treated as port/interface names. The remaining tokens are attribute names
 * (no values — delete always resets to default).
 *
 * Supported delete attributes (valueless):
 *   loopback-mode, lldp-expected-value, lldp-expected-chassis,
 *   lldp-expected-ttl, lldp-expected-port-desc, lldp-expected-system-name,
 *   lldp-expected-system-desc
 */
class InterfaceDeleteAttrs : public utils::BaseObjectArgType<std::string> {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ InterfaceDeleteAttrs(std::vector<std::string> v);

  /* Get the resolved interfaces. */
  const utils::InterfaceList& getInterfaces() const {
    return interfaces_;
  }

  /* Implicit conversion to InterfaceList so that child commands (e.g. ipv6)
   * can accept const InterfaceList& without knowing about InterfaceDeleteAttrs.
   */
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ operator const utils::InterfaceList&() const {
    return interfaces_;
  }

  /* Get the list of attribute names to delete/reset. */
  const std::vector<std::string>& getAttributes() const {
    return attributes_;
  }

  /* Check if any attributes were provided. */
  bool hasAttributes() const {
    return !attributes_.empty();
  }

  const static utils::ObjectArgTypeId id =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_INTERFACE_DELETE_ATTRS;

 private:
  utils::InterfaceList interfaces_;
  std::vector<std::string> attributes_;

  // Check if a string is a known delete attribute name
  static bool isKnownAttribute(const std::string& s);
};

struct CmdDeleteInterfaceTraits : public WriteCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_INTERFACE_DELETE_ATTRS;
  using ObjectArgType = InterfaceDeleteAttrs;
  using RetType = std::string;
};

class CmdDeleteInterface
    : public CmdHandler<CmdDeleteInterface, CmdDeleteInterfaceTraits> {
 public:
  using ObjectArgType = CmdDeleteInterfaceTraits::ObjectArgType;
  using RetType = CmdDeleteInterfaceTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& deleteAttrs);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
