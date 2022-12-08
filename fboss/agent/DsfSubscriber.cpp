// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/DsfSubscriber.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/DsfNode.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/thrift_cow/nodes/Serializer.h"

#include <memory>

namespace facebook::fboss {

using ThriftMapTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::integral,
    apache::thrift::type_class::structure>;
using SysPortMapThriftType = std::map<int64_t, state::SystemPortFields>;
using InterfaceMapThriftType = std::map<int32_t, state::InterfaceFields>;

DsfSubscriber::DsfSubscriber(SwSwitch* sw) : sw_(sw) {
  sw_->registerStateObserver(this, "DSFSubscriber");
}

DsfSubscriber::~DsfSubscriber() {
  stop();
}

void DsfSubscriber::scheduleUpdate(
    const std::shared_ptr<SystemPortMap>& newSysPorts,
    const std::shared_ptr<InterfaceMap>& newRifs,
    const std::string& nodeName,
    SwitchID nodeSwitchId) {
  XLOG(INFO) << " For , switchId: " << static_cast<int64_t>(nodeSwitchId)
             << " got,"
             << " updated # of sys ports: "
             << (newSysPorts ? newSysPorts->size() : 0)
             << " updated # of rifs: " << (newRifs ? newRifs->size() : 0);
  sw_->updateState(
      folly::sformat("Update state for node: {}", nodeName),
      [newSysPorts, newRifs, nodeSwitchId, nodeName](
          const std::shared_ptr<SwitchState>& in) {
        if (nodeSwitchId == SwitchID(*in->getSwitchSettings()->getSwitchId())) {
          throw FbossError(
              " Got updates for my switch ID, from: ",
              nodeName,
              " id: ",
              nodeSwitchId);
        }
        bool changed{false};
        auto out = in->clone();
        auto processDelta = [&](auto& delta, auto& mapToUpdate) {
          DeltaFunctions::forEachChanged(
              delta,
              [&](const auto& oldNode, const auto& newNode) {
                if (*oldNode != *newNode) {
                  // Compare contents as we reconstructed
                  // map from deserialized FSDB
                  // subscriptions. So can't just rely on
                  // pointer comparison here.
                  mapToUpdate->updateNode(newNode);
                  changed = true;
                }
              },
              [&](const auto& newNode) {
                mapToUpdate->addNode(newNode);
                changed = true;
              },
              [&](const auto& rmNode) {
                mapToUpdate->removeNode(rmNode);
                changed = true;
              });
        };
        if (newSysPorts) {
          auto origSysPorts = out->getSystemPorts(nodeSwitchId);
          thrift_cow::ThriftMapDelta<SystemPortMap> delta(
              origSysPorts.get(), newSysPorts.get());
          auto remoteSysPorts = out->getRemoteSystemPorts()->modify(&out);
          processDelta(delta, remoteSysPorts);
        }
        if (newRifs) {
          auto origRifs = out->getInterfaces(nodeSwitchId);
          InterfaceMapDelta delta(origRifs.get(), newRifs.get());
          auto remoteRifs = out->getRemoteInterfaces()->modify(&out);
          processDelta(delta, remoteRifs);
        }
        if (changed) {
          return out;
        }
        return std::shared_ptr<SwitchState>{};
      });
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
        (*node->getLoopbackIps()->cbegin())->toThrift(),
        -1 /*default CIDR*/,
        false /*apply mask*/);
    return network.first.str();
  };
  auto addDsfNode = [&](const std::shared_ptr<DsfNode>& node) {
    // No need to setup subscriptions to (local) yourself
    // Only IN nodes have control plane, so ignore non IN DSF nodes
    if (isLocal(node) || !isInterfaceNode(node)) {
      return;
    }
    auto nodeName = node->getName();
    auto nodeSwitchId = node->getSwitchId();
    XLOG(DBG2) << " Setting up DSF subscriptions to : " << nodeName;
    fsdbPubSubMgr_->addStatePathSubscription(
        {getSystemPortsPath(), getInterfacesPath()},
        [nodeName](auto /*oldState*/, auto newState) {
          XLOG(DBG2) << (newState == fsdb::FsdbStreamClient::State::CONNECTED
                             ? "Connected to:"
                             : "Disconnected from: ")
                     << nodeName;
        },
        [this, nodeName, nodeSwitchId](fsdb::OperSubPathUnit&& operStateUnit) {
          std::shared_ptr<SystemPortMap> newSysPorts;
          std::shared_ptr<InterfaceMap> newRifs;
          for (const auto& change : *operStateUnit.changes()) {
            if (change.path()->path() == getSystemPortsPath()) {
              std::map<int64_t, SystemPortFields> fieldsMap;
              XLOG(DBG2) << " Got sys port update from : " << nodeName;
              newSysPorts = std::make_shared<SystemPortMap>();
              newSysPorts->fromThrift(
                  thrift_cow::
                      deserialize<ThriftMapTypeClass, SysPortMapThriftType>(
                          fsdb::OperProtocol::BINARY,
                          *change.state()->contents()));
            } else if (change.path()->path() == getInterfacesPath()) {
              XLOG(DBG2) << " Got rif update from : " << nodeName;
              newRifs = std::make_shared<InterfaceMap>();
              newRifs->fromThrift(
                  thrift_cow::
                      deserialize<ThriftMapTypeClass, InterfaceMapThriftType>(
                          fsdb::OperProtocol::BINARY,
                          *change.state()->contents()));
            } else {
              throw FbossError(
                  " Got unexpected state update for : ",
                  folly::join("/", *change.path()->path()),
                  " from node: ",
                  nodeName);
            }
          }
          scheduleUpdate(newSysPorts, newRifs, nodeName, nodeSwitchId);
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
        {getSystemPortsPath(), getInterfacesPath()}, getLoopbackIp(node));
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
