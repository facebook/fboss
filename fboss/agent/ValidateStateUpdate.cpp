// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/ValidateStateUpdate.h"

#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

using facebook::fboss::DeltaFunctions::forEachChanged;
using std::shared_ptr;

namespace facebook::fboss {
bool isStateUpdateValidCommon(const StateDelta& delta) {
  bool isValid = true;

  forEachChanged(
      delta.getAclsDelta(),
      [&](const shared_ptr<AclEntry>& /* oldAcl */,
          const shared_ptr<AclEntry>& newAcl) {
        isValid = isValid && newAcl->hasMatcher();
        return isValid ? LoopAction::CONTINUE : LoopAction::BREAK;
      },
      [&](const shared_ptr<AclEntry>& addAcl) {
        isValid = isValid && addAcl->hasMatcher();
        return isValid ? LoopAction::CONTINUE : LoopAction::BREAK;
      },
      [&](const shared_ptr<AclEntry>& /* delAcl */) {});

  forEachChanged(
      delta.getPortsDelta(),
      [&](const shared_ptr<Port>& /* oldport */,
          const shared_ptr<Port>& newport) {
        isValid = isValid && newport->hasValidPortQueues();
        return isValid ? LoopAction::CONTINUE : LoopAction::BREAK;
      },
      [&](const shared_ptr<Port>& addport) {
        isValid = isValid && addport->hasValidPortQueues();
        return isValid ? LoopAction::CONTINUE : LoopAction::BREAK;
      },
      [&](const shared_ptr<Port>& /* delport */) {});

  // Ensure only one sflow mirror session is configured
  std::set<std::string> ingressMirrors;
  for (auto mniter : std::as_const(*(delta.newState()->getPorts()))) {
    for (auto iter : std::as_const(*mniter.second)) {
      auto port = iter.second;
      if (port && port->getIngressMirror().has_value()) {
        auto ingressMirror = delta.newState()->getMirrors()->getNodeIf(
            port->getIngressMirror().value());
        if (ingressMirror && ingressMirror->type() == Mirror::Type::SFLOW) {
          ingressMirrors.insert(port->getIngressMirror().value());
        }
      }
    }
  }

  if (ingressMirrors.size() > 1) {
    XLOG(ERR) << "Only one sflow mirror can be configured across all ports";
    isValid = false;
  }
  return isValid;
}

bool isStateUpdateValidMultiSwitch(const StateDelta& delta) {
  // TODO(pshaikh): Add validation for multi switch
  return true;
}
} // namespace facebook::fboss
