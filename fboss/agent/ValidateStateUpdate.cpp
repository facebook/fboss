// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/ValidateStateUpdate.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
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
        if (!isValid) {
          XLOG(ERR) << "Changed acl " << newAcl->getID() << " has no matcher";
        }
        return isValid ? LoopAction::CONTINUE : LoopAction::BREAK;
      },
      [&](const shared_ptr<AclEntry>& addAcl) {
        isValid = isValid && addAcl->hasMatcher();
        if (!isValid) {
          XLOG(ERR) << "Newly added acl " << addAcl->getID()
                    << " has no matcher";
        }
        return isValid ? LoopAction::CONTINUE : LoopAction::BREAK;
      },
      [&](const shared_ptr<AclEntry>& /* delAcl */) {});

  forEachChanged(
      delta.getPortsDelta(),
      [&](const shared_ptr<Port>& /* oldport */,
          const shared_ptr<Port>& newport) {
        isValid = isValid && newport->hasValidPortQueues();
        if (!isValid) {
          XLOG(ERR) << "Changed port " << newport->getID()
                    << " has invalid queues";
        }

        return isValid ? LoopAction::CONTINUE : LoopAction::BREAK;
      },
      [&](const shared_ptr<Port>& addport) {
        isValid = isValid && addport->hasValidPortQueues();
        if (!isValid) {
          XLOG(ERR) << "Newly added port " << addport->getID()
                    << " has invalid queues";
        }
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

bool isStateUpdateValidMultiSwitch(
    const StateDelta& delta,
    const SwitchIdScopeResolver* resolver,
    const std::map<SwitchID, const HwAsic*>& hwAsics) {
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

  DeltaFunctions::forEachChanged(
      delta.getMirrorsDelta(),
      [&](const std::shared_ptr<Mirror>& /* oldMirror */,
          const std::shared_ptr<Mirror>& newMirror) {
        if (!newMirror->getTruncate()) {
          // the loop mainly verifies truncate
          return LoopAction::CONTINUE;
        }
        auto scope = resolver->scope(newMirror);
        for (auto switchId : scope.switchIds()) {
          auto itr = hwAsics.find(switchId);
          if (itr == hwAsics.end()) {
            XLOG(ERR) << "ASIC not found for the switch " << switchId;
            isValid = false;
            return LoopAction::BREAK;
          }
          if (!itr->second->isSupported(
                  HwAsic::Feature::MIRROR_PACKET_TRUNCATION)) {
            XLOG(ERR)
                << "Mirror packet truncation is not supported on the switch "
                << switchId;
            isValid = false;
            return LoopAction::BREAK;
          }
        }
        return LoopAction::CONTINUE;
      });

  std::map<SwitchID, uint32_t> switchId2Mirrors;
  for (const auto& [matcherStr, mirrors] :
       std::as_const(*(delta.newState()->getMirrors()))) {
    if (!isValid) {
      break;
    }
    HwSwitchMatcher matcher(matcherStr);
    for (const auto& switchId : matcher.switchIds()) {
      if (switchId2Mirrors.find(switchId) == switchId2Mirrors.end()) {
        switchId2Mirrors[switchId] = 0;
      }
      switchId2Mirrors[switchId] += mirrors->size();
      auto itr = hwAsics.find(switchId);
      if (itr == hwAsics.end()) {
        XLOG(ERR) << "ASIC not found for the switch " << switchId;
        isValid = false;
      }
      if (itr->second->getMaxMirrors() < switchId2Mirrors[switchId]) {
        XLOG(ERR) << "Number of mirrors configured "
                  << switchId2Mirrors[switchId]
                  << " is higher on  platform for switch " << switchId
                  << " than the max supported " << itr->second->getMaxMirrors();
        isValid = false;
      }
    }
  }

  return isValid;
}

bool isStateUpdateValidMultiSwitch(
    const StateDelta& delta,
    const SwitchIdScopeResolver* resolver,
    SwitchID switchID,
    const HwAsic* asic) {
  return isStateUpdateValidMultiSwitch(delta, resolver, {{switchID, asic}});
}

} // namespace facebook::fboss
