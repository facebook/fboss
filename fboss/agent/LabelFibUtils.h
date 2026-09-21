// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/agent/state/LabelForwardingEntry.h>

namespace facebook::fboss {

class SwitchState;
class SwitchIdScopeResolver;

std::shared_ptr<SwitchState> programLabel(
    const SwitchIdScopeResolver& resolver,
    const std::shared_ptr<SwitchState>& state,
    Label label,
    ClientID client,
    AdminDistance distance,
    LabelNextHopSet nexthops);

std::shared_ptr<SwitchState> unprogramLabel(
    const SwitchIdScopeResolver& resolver,
    const std::shared_ptr<SwitchState>& state,
    Label label,
    ClientID client);

std::shared_ptr<SwitchState> purgeEntriesForClient(
    const SwitchIdScopeResolver& resolver,
    const std::shared_ptr<SwitchState>& state,
    ClientID client);

} // namespace facebook::fboss
