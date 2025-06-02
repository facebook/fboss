/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/lib/multinode/MultiNodeUtil.h"

#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include "common/network/NetworkUtil.h"

#include "fboss/agent/if/gen-cpp2/FbossCtrlAsyncClient.h"

namespace {
using facebook::fboss::FbossCtrl;

std::unique_ptr<apache::thrift::Client<FbossCtrl>> getSwAgentThriftClient(
    const std::string& switchName) {
  folly::EventBase* eb = folly::EventBaseManager::get()->getEventBase();
  auto remoteSwitchIp =
      facebook::network::NetworkUtil::getHostByName(switchName);
  folly::SocketAddress agent(remoteSwitchIp, 5909);
  auto socket = folly::AsyncSocket::newSocket(eb, agent);
  auto channel =
      apache::thrift::RocketClientChannel::newChannel(std::move(socket));
  return std::make_unique<apache::thrift::Client<FbossCtrl>>(
      std::move(channel));
}

} // namespace

namespace facebook::fboss::utility {

MultiNodeUtil::MultiNodeUtil(
    const std::shared_ptr<MultiSwitchDsfNodeMap>& dsfNodeMap) {
  populateDsfNodes(dsfNodeMap);
}

void MultiNodeUtil::populateDsfNodes(
    const std::shared_ptr<MultiSwitchDsfNodeMap>& dsfNodeMap) {
  for (const auto& [_, dsfNodes] : std::as_const(*dsfNodeMap)) {
    for (const auto& [_, node] : std::as_const(*dsfNodes)) {
      switchIdToSwitchName_[node->getSwitchId()] = node->getName();

      if (node->getType() == cfg::DsfNodeType::INTERFACE_NODE) {
        CHECK(node->getClusterId().has_value());
        auto clusterId = node->getClusterId().value();
        clusterIdToRdsws_[clusterId].push_back(node->getName());
      } else if (node->getType() == cfg::DsfNodeType::FABRIC_NODE) {
        CHECK(node->getFabricLevel().has_value());
        if (node->getFabricLevel().value() == 1) {
          CHECK(node->getClusterId().has_value());
          auto clusterId = node->getClusterId().value();
          clusterIdToFdsws_[clusterId].push_back(node->getName());
        } else if (node->getFabricLevel().value() == 2) {
          sdsws_.insert(node->getName());
        } else {
          XLOG(FATAL) << "Invalid fabric level"
                      << node->getFabricLevel().value();
        }
      } else {
        XLOG(FATAL) << "Invalid DSF Node type"
                    << apache::thrift::util::enumNameSafe(node->getType());
      }
    }
  }
}

std::set<std::string> MultiNodeUtil::getAllRdsws() {
  std::set<std::string> allRdsws;
  for (const auto& [clusterId, rdsws] : std::as_const(clusterIdToRdsws_)) {
    for (const auto& rdsw : rdsws) {
      allRdsws.insert(rdsw);
    }
  }
  return allRdsws;
}

std::set<std::string> MultiNodeUtil::getGlobalSystemPortsOfType(
    const std::string& rdsw,
    const std::set<RemoteSystemPortType>& types) {
  auto logSystemPort =
      [rdsw](const facebook::fboss::SystemPortThrift& systemPort) {
        XLOG(DBG2)
            << "From " << rdsw << " portId: " << systemPort.portId().value()
            << " switchId: " << systemPort.switchId().value()
            << " portName: " << systemPort.portName().value()
            << " remoteSystemPortType: "
            << apache::thrift::util::enumNameSafe(
                   systemPort.remoteSystemPortType().value_or(-1))
            << " remoteSystemPortLivenessStatus: "
            << folly::to<std::string>(
                   systemPort.remoteSystemPortLivenessStatus().value_or(-1))
            << " scope: "
            << apache::thrift::util::enumNameSafe(systemPort.scope().value());
      };

  auto swAgentClient = getSwAgentThriftClient(rdsw);
  std::map<int64_t, facebook::fboss::SystemPortThrift> systemPortEntries;
  swAgentClient->sync_getSystemPorts(systemPortEntries);

  auto matchesPortType =
      [&types](const facebook::fboss::SystemPortThrift& systemPort) {
        if (systemPort.remoteSystemPortType().has_value()) {
          return types.find(systemPort.remoteSystemPortType().value()) !=
              types.end();
        } else {
          return types.empty();
        }
      };

  std::set<std::string> systemPortsOfType;
  for (const auto& [_, systemPort] : systemPortEntries) {
    logSystemPort(systemPort);
    if (*systemPort.scope() == cfg::Scope::GLOBAL &&
        matchesPortType(systemPort)) {
      systemPortsOfType.insert(systemPort.portName().value());
    }
  }

  return systemPortsOfType;
}

bool MultiNodeUtil::verifySystemPortsForRdsw(const std::string& rdswToVerify) {
  // Every GLOBAL system port of every remote RDSW is either a STATIC_ENTRY or
  // DYNAMIC_ENTRY of the local RDSW
  std::set<std::string> gotSystemPorts;
  std::set<std::string> expectedSystemPorts;
  for (const auto& [clusterId, rdsws] : std::as_const(clusterIdToRdsws_)) {
    for (const auto& rdsw : std::as_const(rdsws)) {
      if (rdsw == rdswToVerify) {
        gotSystemPorts = getGlobalSystemPortsOfType(
            rdswToVerify,
            {RemoteSystemPortType::STATIC_ENTRY,
             RemoteSystemPortType::
                 DYNAMIC_ENTRY} /* Get Global Remote ports of self */);
      } else {
        auto systemPortsForRdsw = getGlobalSystemPortsOfType(
            rdsw, {} /* Get Global ports of Remote RDSW */);
        expectedSystemPorts.insert(
            systemPortsForRdsw.begin(), systemPortsForRdsw.end());
      }
    }
  }

  XLOG(DBG2) << "From " << rdswToVerify << " Expected System Ports: "
             << folly::join(",", expectedSystemPorts)
             << " Got System Ports: " << folly::join(",", gotSystemPorts);

  return expectedSystemPorts == gotSystemPorts;
}

bool MultiNodeUtil::verifySystemPorts() {
  for (const auto& rdsw : getAllRdsws()) {
    if (!verifySystemPortsForRdsw(rdsw)) {
      return false;
    }
  }

  return true;
}

std::set<int> MultiNodeUtil::getGlobalRifsOfType(
    const std::string& rdsw,
    const std::set<RemoteInterfaceType>& types) {
  auto logRif = [rdsw](const facebook::fboss::InterfaceDetail& rif) {
    XLOG(DBG2)
        << "From " << rdsw << " interfaceName: " << rif.interfaceName().value()
        << " interfaceId: " << rif.interfaceId().value() << " remoteIntfType: "
        << apache::thrift::util::enumNameSafe(rif.remoteIntfType().value_or(-1))
        << " remoteIntfLivenessStatus: "
        << folly::to<std::string>(rif.remoteIntfLivenessStatus().value_or(-1))
        << " scope: "
        << apache::thrift::util::enumNameSafe(rif.scope().value());
  };

  auto swAgentClient = getSwAgentThriftClient(rdsw);
  std::map<int32_t, facebook::fboss::InterfaceDetail> rifs;
  swAgentClient->sync_getAllInterfaces(rifs);

  auto matchesRifType = [&types](const facebook::fboss::InterfaceDetail& rif) {
    if (rif.remoteIntfType().has_value()) {
      return types.find(rif.remoteIntfType().value()) != types.end();
    } else {
      return types.empty();
    }
  };

  std::set<int> rifsOfType;
  for (const auto& [_, rif] : rifs) {
    logRif(rif);
    if (*rif.scope() == cfg::Scope::GLOBAL && matchesRifType(rif)) {
      rifsOfType.insert(rif.interfaceId().value());
    }
  }

  return rifsOfType;
}

bool MultiNodeUtil::verifyRifsForRdsw(const std::string& rdswToVerify) {
  // Every GLOBAL rif of every remote RDSW is either a STATIC_ENTRY or
  // DYNAMIC_ENTRY of the local RDSW
  std::set<int> gotRifs;
  std::set<int> expectedRifs;
  for (const auto& [clusterId, rdsws] : std::as_const(clusterIdToRdsws_)) {
    for (const auto& rdsw : std::as_const(rdsws)) {
      if (rdsw == rdswToVerify) {
        gotRifs = getGlobalRifsOfType(
            rdswToVerify,
            {RemoteInterfaceType::STATIC_ENTRY,
             RemoteInterfaceType::
                 DYNAMIC_ENTRY} /* Get Global Remote rifs of self */);
      } else {
        auto rifsForRdsw =
            getGlobalRifsOfType(rdsw, {} /* Get Global rifs of Remote RDSW */);
        expectedRifs.insert(rifsForRdsw.begin(), rifsForRdsw.end());
      }
    }
  }

  XLOG(DBG2) << "From " << rdswToVerify
             << " Expected Rifs: " << folly::join(",", expectedRifs)
             << " Got Rifs: " << folly::join(",", gotRifs);

  return expectedRifs == gotRifs;
}

bool MultiNodeUtil::verifyRifs() {
  for (const auto& rdsw : getAllRdsws()) {
    if (!verifyRifsForRdsw(rdsw)) {
      return false;
    }
  }

  return true;
}

std::set<std::pair<std::string, std::string>>
MultiNodeUtil::getNdpEntriesAndSwitchOfType(
    const std::string& rdsw,
    const std::set<std::string>& types) {
  auto swAgentClient = getSwAgentThriftClient(rdsw);
  std::vector<facebook::fboss::NdpEntryThrift> ndpEntries;
  swAgentClient->sync_getNdpTable(ndpEntries);

  auto matchesNdpType =
      [&types](const facebook::fboss::NdpEntryThrift& ndpEntry) {
        return types.find(ndpEntry.state().value()) != types.end();
      };

  std::set<std::pair<std::string, std::string>> ndpEntriesAndSwitchOfType;
  for (const auto& ndpEntry : ndpEntries) {
    if (matchesNdpType(ndpEntry)) {
      CHECK(ndpEntry.switchId().has_value());
      CHECK(
          switchIdToSwitchName_.find(SwitchID(ndpEntry.switchId().value())) !=
          std::end(switchIdToSwitchName_));

      auto ip = folly::IPAddress::fromBinary(folly::ByteRange(
          folly::StringPiece(ndpEntry.ip().value().addr().value())));

      ndpEntriesAndSwitchOfType.insert(std::make_pair(
          ip.str(),
          switchIdToSwitchName_[SwitchID(ndpEntry.switchId().value())]));
    }
  }

  return ndpEntriesAndSwitchOfType;
}

std::set<std::string> MultiNodeUtil::getRdswsWithEstablishedDsfSessions(
    const std::string& rdsw) {
  auto logDsfSession =
      [rdsw](const facebook::fboss::DsfSessionThrift& session) {
        XLOG(DBG2) << "From " << rdsw << " session: " << *session.remoteName()
                   << " state: "
                   << apache::thrift::util::enumNameSafe(*session.state())
                   << " lastEstablishedAt: "
                   << session.lastEstablishedAt().value_or(0)
                   << " lastDisconnectedAt: "
                   << session.lastDisconnectedAt().value_or(0);
      };

  auto swAgentClient = getSwAgentThriftClient(rdsw);
  std::vector<facebook::fboss::DsfSessionThrift> sessions;
  swAgentClient->sync_getDsfSessions(sessions);

  std::set<std::string> gotRdsws;
  for (const auto& session : sessions) {
    logDsfSession(session);
    if (session.state() == facebook::fboss::DsfSessionState::ESTABLISHED) {
      size_t pos = (*session.remoteName()).find("::");
      if (pos != std::string::npos) {
        gotRdsws.insert((*session.remoteName()).substr(0, pos));
      }
    }
  }

  return gotRdsws;
}

bool MultiNodeUtil::verifyDsfSessions() {
  // Every RDSW must have an ESTABLISHED DSF Session with every other RDSW
  auto allRdsws = getAllRdsws();

  for (const auto& [clusterId, rdsws] : std::as_const(clusterIdToRdsws_)) {
    for (const auto& rdsw : std::as_const(rdsws)) {
      auto expectedRdsws = allRdsws;
      expectedRdsws.erase(rdsw); // exclude self
      auto gotRdsws = getRdswsWithEstablishedDsfSessions(rdsw);
      if (expectedRdsws != gotRdsws) {
        XLOG(DBG2) << "DSF ESTABLISHED sessions from " << rdsw
                   << " Expected RDSWs: " << folly::join(",", expectedRdsws)
                   << " Got RDSWs: " << folly::join(",", gotRdsws);
        return false;
      }
    }
  }

  return true;
}

} // namespace facebook::fboss::utility
