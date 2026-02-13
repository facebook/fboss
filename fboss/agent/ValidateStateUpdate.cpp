// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/ValidateStateUpdate.h"

#include "fboss/agent/FbossError.h"
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
  auto globalQosDelta = delta.getDefaultDataPlaneQosPolicyDelta();
  auto isValid = true;
  if (globalQosDelta.getNew()) {
    auto& newPolicy = globalQosDelta.getNew();
    bool hasDscpMap =
        newPolicy->getDscpMap()->get<switch_state_tags::from>()->size() > 0;
    auto pcpMap = newPolicy->getPcpMap();
    bool hasPcpMap = pcpMap.has_value() && !pcpMap->empty();
    bool hasTcToQueue = newPolicy->getTrafficClassToQueueId()->size() > 0;

    if ((!hasDscpMap && !hasPcpMap) || !hasTcToQueue) {
      XLOG(ERR)
          << " Either DSCP to TC or PCP to TC map, along with TC to Queue map, must be provided in valid qos policies";
      return false;
    }
    /*
     * Not adding a check for expMap even though we don't support
     * MPLS QoS yet. Unfortunately, SwSwitch implicitly sets a exp map
     * even if the config doesn't have one. So no point in warning/failing
     * on what could be just system generated behavior.
     * TODO: see if we can stop doing this at SwSwitch layre
     */
  }

  if (delta.oldState()->getSwitchSettings()->size() &&
      delta.newState()->getSwitchSettings()->empty()) {
    throw FbossError("Switch settings cannot be removed from SwitchState");
  }

  // Only single watchdog recovery action is supported.
  // TODO - Add support for per port watchdog recovery action
  std::shared_ptr<Port> firstPort;
  std::optional<cfg::PfcWatchdogRecoveryAction> recoveryAction{};
  for (const auto& portMap : std::as_const(*delta.newState()->getPorts())) {
    for (const auto& port : std::as_const(*portMap.second)) {
      if (port.second->getPfc().has_value() &&
          port.second->getPfc()->watchdog().has_value()) {
        auto pfcWd = port.second->getPfc()->watchdog().value();
        if (!recoveryAction.has_value()) {
          recoveryAction = *pfcWd.recoveryAction();
          firstPort = port.second;
        } else if (*recoveryAction != *pfcWd.recoveryAction()) {
          // Error: All ports should have the same recovery action configured
          XLOG(ERR) << "PFC watchdog deadlock recovery action on "
                    << port.second->getName() << " conflicting with "
                    << firstPort->getName();
          isValid = false;
        }
      }
    }
  }
  return isValid;
}
} // namespace facebook::fboss
