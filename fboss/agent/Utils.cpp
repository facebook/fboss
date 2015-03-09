/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/Utils.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/SysError.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"

#include <boost/filesystem/operations.hpp>

using folly::IPAddressV4;
using folly::IPAddressV6;

namespace facebook { namespace fboss {

void utilCreateDir(folly::StringPiece path) {
  try {
    boost::filesystem::create_directories(path.str());
  } catch (...) {
    throw SysError(errno, "failed to create directory \"", path, "\"");
  }
}

IPAddressV4 getSwitchVlanIP(const std::shared_ptr<SwitchState>& state,
                            VlanID vlan) {
  IPAddressV4 switchIp;
  auto vlanInterfaces = state->getInterfaces()->getInterfacesInVlanIf(vlan);
  for (const auto& vlanIntf: vlanInterfaces) {
    auto& addresses = vlanIntf->getAddresses();
    for (const auto& address: addresses) {
      if (address.first.isV4()) {
        switchIp = address.first.asV4();
        return switchIp;
      }
    }
  }
  throw FbossError("Cannot find IP address for vlan", vlan);
}

IPAddressV6 getSwitchVlanIPv6(const std::shared_ptr<SwitchState>& state,
                              VlanID vlan) {
  IPAddressV6 switchIp;
  auto vlanInterfaces = state->getInterfaces()->getInterfacesInVlanIf(vlan);
  for (const auto& vlanIntf: vlanInterfaces) {
    auto& addresses = vlanIntf->getAddresses();
    for (const auto& address: addresses) {
      if (address.first.isV6()) {
        switchIp = address.first.asV6();
        return switchIp;
      }
    }
  }
  throw FbossError("Cannot find IPv6 address for vlan ", vlan);
}

}} // facebook::fboss
