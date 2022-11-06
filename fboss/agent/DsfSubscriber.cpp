// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/DsfSubscriber.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/DsfNode.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"

#include <memory>

namespace facebook::fboss {

DsfSubscriber::DsfSubscriber(SwSwitch* sw) : sw_(sw) {
  sw_->registerStateObserver(this, "DSFSubscriber");
}

DsfSubscriber::~DsfSubscriber() {
  stop();
}

void DsfSubscriber::stateUpdated(const StateDelta& stateDelta) {
  const auto& oldSwitchSettings = stateDelta.oldState()->getSwitchSettings();
  const auto& newSwitchSettings = stateDelta.newState()->getSwitchSettings();
  if (newSwitchSettings->getSwitchType() == cfg::SwitchType::VOQ) {
    if (!fsdbPubSubMgr_) {
      fsdbPubSubMgr_ = std::make_unique<fsdb::FsdbPubSubManager>("agent");
    }
  } else {
    fsdbPubSubMgr_.reset();
    if (oldSwitchSettings->getSwitchType() == cfg::SwitchType::VOQ) {
      XLOG(FATAL)
          << " Transition from VOQ to non-VOQ swtich type is not supported";
    }
  }
  auto mySwitchId = newSwitchSettings->getSwitchId();

  auto isLocal = [mySwitchId](const std::shared_ptr<DsfNode>& node) {
    CHECK(mySwitchId) << " Dsf node config requires local switch ID to be set";
    return SwitchID(*mySwitchId) == node->getSwitchId();
  };
  auto isInterfaceNode = [](const std::shared_ptr<DsfNode>& node) {
    return node->getType() == cfg::DsfNodeType::INTERFACE_NODE;
  };
  auto getLoopbackIp = [](const std::shared_ptr<DsfNode>& node) {
    auto network = folly::IPAddress::createNetwork(
        (*node->getLoopbackIps()->cbegin())->toThrift());
    return network.first.str();
  };
  auto addDsfNode = [&](const std::shared_ptr<DsfNode>& node) {
    // No need to setup subscriptions to (local) yourself
    // Only IN nodes have control plane, so ignore non IN DSF nodes
    if (isLocal(node) || !isInterfaceNode(node)) {
      return;
    }
    auto nodeName = node->getName();
    XLOG(DBG2) << " Setting up DSF subscriptions to : " << nodeName;
    fsdbPubSubMgr_->addStatePathSubscription(
        getSystemPortsPath(),
        [](auto /*oldState*/, auto /*newState*/) {},
        [nodeName](auto /*operStateUnit*/) {
          XLOG(DBG2) << " Got system ports update from: " << nodeName;
        },
        getLoopbackIp(node));
    fsdbPubSubMgr_->addStatePathSubscription(
        getInterfacesPath(),
        [](auto /*oldState*/, auto /*newState*/) {},
        [nodeName](auto /*operStateUnit*/) {
          XLOG(DBG2) << " Got interfaces update from: " << nodeName;
        },
        getLoopbackIp(node));
  };
  auto rmDsfNode = [&](const std::shared_ptr<DsfNode>& node) {
    // No need to setup subscriptions to (local) yourself
    // Only IN nodes have control plane, so ignore non IN DSF nodes
    if (isLocal(node) || !isInterfaceNode(node)) {
      return;
    }
    XLOG(DBG2) << " Removing DSF subscriptions to : " << node->getName();
    fsdbPubSubMgr_->removeStatePathSubscription(
        getSystemPortsPath(), getLoopbackIp(node));
    fsdbPubSubMgr_->removeStatePathSubscription(
        getInterfacesPath(), getLoopbackIp(node));
  };
  DeltaFunctions::forEachChanged(
      stateDelta.getDsfNodesDelta(),
      [&](auto oldNode, auto newNode) {
        rmDsfNode(oldNode);
        addDsfNode(newNode);
      },
      addDsfNode,
      rmDsfNode);
}

void DsfSubscriber::stop() {
  sw_->unregisterStateObserver(this);
  fsdbPubSubMgr_.reset();
}

} // namespace facebook::fboss
