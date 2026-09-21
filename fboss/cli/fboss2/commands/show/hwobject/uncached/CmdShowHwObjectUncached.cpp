/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/hwobject/uncached/CmdShowHwObjectUncached.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

namespace facebook::fboss {

CmdShowHwObjectUncached::RetType CmdShowHwObjectUncached::queryClient(
    const HostInfo& hostInfo,
    const ObjectArgType& queriedHwObjectTypes) {
  return queryHwObjects(hostInfo, queriedHwObjectTypes, false /*cached*/);
}

void CmdShowHwObjectUncached::printOutput(
    const RetType& hwObjectInfo,
    std::ostream& out) {
  out << hwObjectInfo << std::endl;
}

std::string_view CmdShowHwObjectUncachedTraits::description() {
  return "Displays the raw SAI hardware-object state for a given object type, read directly from the ASIC instead of the agent's cached copy. Requires an object-type argument (the same set as `show hw-object`); note the object type is given before `uncached`, e.g. `show hw-object PORT uncached`. Use it when the cached hardware-object state is suspected to be stale.";
}

CmdShowHwObjectUncached::RetType CmdShowHwObjectUncached::sampleModel() {
  return R"(Object type: port
PortSaiId(4294967399): (HwLaneList: [185, 186], Speed: 50000, AdminState: true, FecMode: 1, PortLoopbackMode: 0, MediaType: 3, GlobalFlowControlMode: 0, PortVlanId: 2000, Mtu: 9412, PrbsPolynomial: 31, PrbsConfig: 0, PtpMode: 1, InterFrameGap: 96, LinkTrainingEnable: false, FdrEnable: false, ...)
PortSaiId(4294967303): (HwLaneList: [27, 28], Speed: 50000, AdminState: true, FecMode: 1, PortLoopbackMode: 0, MediaType: 3, GlobalFlowControlMode: 0, PortVlanId: 2000, Mtu: 9412, PrbsPolynomial: 31, PrbsConfig: 0, PtpMode: 1, InterFrameGap: 96, LinkTrainingEnable: false, FdrEnable: false, ...)

Object type: port-serdes
PortSerdesSaiId(6755773103210496): (PortId: 4294967320, Preemphasis: [4294967282, 4294967282], IDriver: [4294967295, 4294967295], TxFirPre1: [4294967282, 4294967282], TxFirPre2: [0, 0], TxFirMain: [109, 109], TxFirPost1: [4294967292, 4294967292], TxFirPost2: [0, 0], TxFirPost3: [0, 0], ...)
)";
}

// Explicit template instantiation
template void
CmdHandler<CmdShowHwObjectUncached, CmdShowHwObjectUncachedTraits>::run();
} // namespace facebook::fboss
