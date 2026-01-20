/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/qsfp_service/QsfpServiceThreads.h"

#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

namespace facebook::fboss {

std::shared_ptr<QsfpServiceThreads> createQsfpServiceThreads(
    const std::shared_ptr<const PlatformMapping>& platformMapping) {
  auto qsfpServiceThreads = std::make_shared<QsfpServiceThreads>();
  const auto& chips = platformMapping->getChips();
  int nextThreadId = 0;

  // Iterate over all ports.
  for (const auto& [portIdInt, portEntry] :
       platformMapping->getPlatformPorts()) {
    PortID portId = PortID(portIdInt);

    // Get transceivers and xphys for this port
    auto tcvrChips = utility::getDataPlanePhyChips(
        portEntry, chips, phy::DataPlanePhyChipType::TRANSCEIVER);
    auto xphyChips = utility::getDataPlanePhyChips(
        portEntry, chips, phy::DataPlanePhyChipType::XPHY);

    // Extract IDs (may be empty)
    std::optional<TransceiverID> tcvrId;
    std::optional<XphyId> xphyId;

    if (!tcvrChips.empty()) {
      tcvrId = TransceiverID(*tcvrChips.begin()->second.physicalID());
    }
    if (!xphyChips.empty()) {
      xphyId = XphyId(*xphyChips.begin()->second.physicalID());
    }

    // Skip ports without transceiver or xphy
    if (!tcvrId && !xphyId) {
      continue;
    }

    // Check if tcvr already has a thread
    auto tcvrIt = tcvrId ? qsfpServiceThreads->tcvrToThreadId.find(*tcvrId)
                         : qsfpServiceThreads->tcvrToThreadId.end();
    bool tcvrHasThread =
        tcvrId && tcvrIt != qsfpServiceThreads->tcvrToThreadId.end();

    // Check if xphy already has a thread
    auto xphyIt = xphyId ? qsfpServiceThreads->xphyToThreadId.find(*xphyId)
                         : qsfpServiceThreads->xphyToThreadId.end();
    bool xphyHasThread =
        xphyId && xphyIt != qsfpServiceThreads->xphyToThreadId.end();

    int threadId;

    if (tcvrId && !xphyId) {
      // Port has tcvr but no xphy
      if (tcvrHasThread) {
        threadId = tcvrIt->second;
      } else {
        threadId = nextThreadId++;
        qsfpServiceThreads->threadIdToThread.emplace(
            threadId, SlotThreadHelper(threadId));
        qsfpServiceThreads->tcvrToThreadId[*tcvrId] = threadId;
        XLOG(ERR) << "Creating thread " << threadId << " for tcvr " << *tcvrId;
      }
    } else if (xphyId && !tcvrId) {
      // Port has xphy but no tcvr
      if (xphyHasThread) {
        threadId = xphyIt->second;
      } else {
        threadId = nextThreadId++;
        qsfpServiceThreads->threadIdToThread.emplace(
            threadId, SlotThreadHelper(threadId));
        qsfpServiceThreads->xphyToThreadId[*xphyId] = threadId;
        XLOG(ERR) << "Creating thread " << threadId << " for xphy " << *xphyId;
      }
    } else {
      // Port has both tcvr and xphy
      if (tcvrHasThread && !xphyHasThread) {
        threadId = tcvrIt->second;
        qsfpServiceThreads->xphyToThreadId[*xphyId] = threadId;
      } else if (xphyHasThread && !tcvrHasThread) {
        threadId = xphyIt->second;
        qsfpServiceThreads->tcvrToThreadId[*tcvrId] = threadId;
      } else if (!tcvrHasThread && !xphyHasThread) {
        threadId = nextThreadId++;
        qsfpServiceThreads->threadIdToThread.emplace(
            threadId, SlotThreadHelper(threadId));
        qsfpServiceThreads->tcvrToThreadId[*tcvrId] = threadId;
        qsfpServiceThreads->xphyToThreadId[*xphyId] = threadId;
        XLOG(ERR) << "Creating thread " << threadId << " for tcvr " << *tcvrId
                  << " and xphy " << *xphyId;
      } else {
        // Both already have threads - verify they are equivalent
        if (tcvrIt->second != xphyIt->second) {
          throw FbossError(
              "Port ",
              portId,
              ": Tcvr ",
              *tcvrId,
              " and Xphy ",
              *xphyId,
              " are already assigned to different threads");
        }
        threadId = tcvrIt->second;
      }
    }

    // Map this port to the thread
    qsfpServiceThreads->portToThreadId[portId] = threadId;
  }

  // Iterate over all transceiverIDs for those not covered in ports (primarily
  // for test fixtures).
  for (const auto& tcvrId : utility::getTransceiverIds(chips)) {
    if (qsfpServiceThreads->tcvrToThreadId.find(tcvrId) !=
        qsfpServiceThreads->tcvrToThreadId.end()) {
      continue;
    }

    int threadId = nextThreadId++;
    qsfpServiceThreads->threadIdToThread.emplace(
        threadId, SlotThreadHelper(threadId));
    qsfpServiceThreads->tcvrToThreadId[tcvrId] = threadId;
    XLOG(ERR) << "Creating thread " << threadId << " for transceiver "
              << tcvrId;
  }

  return qsfpServiceThreads;
}

folly::EventBase* getEventBaseForTcvr(
    const std::shared_ptr<QsfpServiceThreads>& threads,
    TransceiverID tcvrId) {
  auto tcvrIt = threads->tcvrToThreadId.find(tcvrId);
  if (tcvrIt == threads->tcvrToThreadId.end()) {
    return nullptr;
  }

  auto threadIt = threads->threadIdToThread.find(tcvrIt->second);
  if (threadIt == threads->threadIdToThread.end()) {
    return nullptr;
  }

  return threadIt->second.getEventBase();
}

folly::EventBase* getEventBaseForPort(
    const std::shared_ptr<QsfpServiceThreads>& threads,
    PortID portId) {
  auto portIt = threads->portToThreadId.find(portId);
  if (portIt == threads->portToThreadId.end()) {
    return nullptr;
  }

  auto threadIt = threads->threadIdToThread.find(portIt->second);
  if (threadIt == threads->threadIdToThread.end()) {
    return nullptr;
  }

  return threadIt->second.getEventBase();
}

} // namespace facebook::fboss
