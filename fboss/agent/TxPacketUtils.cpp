// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/TxPacketUtils.h"

namespace facebook::fboss::utility {

template std::optional<VlanID> getVlanIDForTx<Vlan>(
    const std::shared_ptr<Vlan>& vlanOrIntf,
    const std::shared_ptr<SwitchSettings>& settings,
    const HwAsic* hwAsic);

template std::optional<VlanID> getVlanIDForTx<Interface>(
    const std::shared_ptr<Interface>& vlanOrIntf,
    const std::shared_ptr<SwitchSettings>& settings,
    const HwAsic* hwAsic);

template std::optional<VlanID> getVlanIDForTx<Vlan>(
    const std::shared_ptr<Vlan>& vlanOrIntf,
    const std::shared_ptr<SwitchState>& state,
    const SwitchIdScopeResolver* resolver,
    const HwAsicTable* asicTable);

template std::optional<VlanID> getVlanIDForTx<Interface>(
    const std::shared_ptr<Interface>& vlanOrIntf,
    const std::shared_ptr<SwitchState>& state,
    const SwitchIdScopeResolver* resolver,
    const HwAsicTable* asicTable);

} // namespace facebook::fboss::utility
