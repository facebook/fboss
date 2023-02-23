// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/DsfSubscriber.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/state/DsfNode.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/common/Flags.h"
#include "fboss/thrift_cow/nodes/Serializer.h"

#include <memory>

DEFINE_bool(dsf_subscriber_skip_hw_writes, false, "Skip writing to HW");
DEFINE_bool(
    dsf_subscriber_cache_updated_state,
    false,
    "Cache switch state after update by dsf subsriber");

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
  XLOG(DBG2) << " For , switchId: " << static_cast<int64_t>(nodeSwitchId)
             << " got,"
             << " updated # of sys ports: "
             << (newSysPorts ? newSysPorts->size() : 0)
             << " updated # of rifs: " << (newRifs ? newRifs->size() : 0);
  sw_->updateState(
      folly::sformat("Update state for node: {}", nodeName),
      [this, newSysPorts, newRifs, nodeSwitchId, nodeName](
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

        auto makeRemoteSysPort = [&](const auto& node) { return node; };
        auto makeRemoteRif = [&](const auto& node) {
          auto clonedNode = node->clone();

          if (node->isPublished()) {
            clonedNode->setArpTable(node->getArpTable()->toThrift());
            clonedNode->setNdpTable(node->getNdpTable()->toThrift());
          }

          // Local neighbor entry on one DSF node is remote neighbor entry on
          // every other DSF node. Thus, for neighbor entry received from other
          // DSF nodes, set isLocal = False before programming it.
          for (const auto& arpEntry : *clonedNode->getArpTable()) {
            arpEntry.second->setIsLocal(false);
          }
          for (const auto& ndpEntry : *clonedNode->getNdpTable()) {
            ndpEntry.second->setIsLocal(false);
          }

          return clonedNode;
        };

        auto processDelta =
            [&](auto& delta, auto& mapToUpdate, auto& makeRemote) {
              DeltaFunctions::forEachChanged(
                  delta,
                  [&](const auto& oldNode, const auto& newNode) {
                    if (*oldNode != *newNode) {
                      // Compare contents as we reconstructed
                      // map from deserialized FSDB
                      // subscriptions. So can't just rely on
                      // pointer comparison here.
                      auto clonedNode = makeRemote(newNode);
                      mapToUpdate->updateNode(clonedNode);
                      changed = true;
                    }
                  },
                  [&](const auto& newNode) {
                    auto clonedNode = makeRemote(newNode);
                    mapToUpdate->addNode(clonedNode);
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
          processDelta(delta, remoteSysPorts, makeRemoteSysPort);
        }
        if (newRifs) {
          auto origRifs = out->getInterfaces(nodeSwitchId);
          InterfaceMapDelta delta(origRifs.get(), newRifs.get());
          auto remoteRifs = out->getRemoteInterfaces()->modify(&out);
          processDelta(delta, remoteRifs, makeRemoteRif);
        }
        if (FLAGS_dsf_subscriber_cache_updated_state) {
          cachedState_ = out;
        }
        if (changed && !FLAGS_dsf_subscriber_skip_hw_writes) {
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
      fsdbPubSubMgr_ = std::make_unique<fsdb::FsdbPubSubManager>(
          folly::sformat("{}:agent", getLocalHostname()));
    }
  } else {
    fsdbPubSubMgr_.reset();
    if (oldSwitchSettings->getSwitchType() == cfg::SwitchType::VOQ) {
      XLOG(FATAL)
          << " Transition from VOQ to non-VOQ swtich type is not supported";
    }
    // No processing needed on non VOQ switches
    return;
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

  auto getServerOptions = [mySwitchId, getLoopbackIp](
                              const std::shared_ptr<DsfNode>& node,
                              const auto& state) {
    auto selfDsfNode = state->getDsfNodes()->getNodeIf(*mySwitchId);
    CHECK(selfDsfNode);
    CHECK(selfDsfNode->getLoopbackIpsSorted().size() != 0);

    // Subscribe to FSDB of DSF node in the cluster with:
    //  dstIP = inband IP of that DSF node
    //  dstPort = FSDB port
    //  srcIP = self inband IP
    auto serverOptions = fsdb::FsdbStreamClient::ServerOptions(
        getLoopbackIp(node),
        FLAGS_fsdbPort,
        (*selfDsfNode->getLoopbackIpsSorted().begin()).first.str());

    return serverOptions;
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
        getServerOptions(node, stateDelta.newState()));
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
