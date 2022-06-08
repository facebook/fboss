// Copyright 2004-present Facebook. All Rights Reserved.
#include "fboss/agent/lldp/LinkNeighborDB.h"

using std::lock_guard;
using std::vector;
using std::chrono::steady_clock;

namespace facebook::fboss {

LinkNeighborDB::NeighborKey::NeighborKey(const LinkNeighbor& neighbor)
    : chassisIdType_(neighbor.getChassisIdType()),
      portIdType_(neighbor.getPortIdType()),
      chassisId_(neighbor.getChassisId()),
      portId_(neighbor.getPortId()) {}

bool LinkNeighborDB::NeighborKey::operator<(const NeighborKey& other) const {
  return std::tie(chassisIdType_, portIdType_, chassisId_, portId_) <
      std::tie(
             other.chassisIdType_,
             other.portIdType_,
             other.chassisId_,
             other.portId_);
}

bool LinkNeighborDB::NeighborKey::operator==(const NeighborKey& other) const {
  return std::tie(chassisIdType_, portIdType_, chassisId_, portId_) ==
      std::tie(
             other.chassisIdType_,
             other.portIdType_,
             other.chassisId_,
             other.portId_);
}

LinkNeighborDB::LinkNeighborDB() {}

void LinkNeighborDB::update(const LinkNeighbor& neighbor) {
  auto neighborsLocked = byLocalPort_.wlock();
  // Go ahead and prune expired neighbors each time we get updated.
  pruneLocked(*neighborsLocked, steady_clock::now());
  auto neighborMap =
      neighborsLocked->try_emplace(neighbor.getLocalPort(), NeighborMap());
  neighborMap.first->second[NeighborKey(neighbor)] = neighbor;
}

vector<LinkNeighbor> LinkNeighborDB::getNeighbors() {
  vector<LinkNeighbor> results;
  auto neighborsLocked = byLocalPort_.rlock();
  for (const auto& portEntry : *neighborsLocked) {
    for (const auto& entry : portEntry.second) {
      results.push_back(entry.second);
    }
  }

  return results;
}

vector<LinkNeighbor> LinkNeighborDB::getNeighbors(PortID port) {
  vector<LinkNeighbor> results;
  auto neighborsLocked = byLocalPort_.rlock();
  if (auto it = neighborsLocked->find(port); it != neighborsLocked->end()) {
    for (const auto& entry : it->second) {
      results.push_back(entry.second);
    }
  }

  return results;
}

int LinkNeighborDB::pruneExpiredNeighbors() {
  return pruneExpiredNeighbors(steady_clock::now());
}

int LinkNeighborDB::pruneExpiredNeighbors(steady_clock::time_point now) {
  return byLocalPort_.withWLock(
      [this, now](auto& neighborMap) { return pruneLocked(neighborMap, now); });
}

void LinkNeighborDB::portDown(PortID port) {
  byLocalPort_.withWLock([port](auto& neighborMap) {
    // Port went down, prune lldp entries for that port
    neighborMap.erase(port);
  });
}

int LinkNeighborDB::pruneLocked(
    NeighborsByPort& neighborsByPort,
    std::chrono::steady_clock::time_point now) {
  // We just do a linear scan for now.
  //
  // We could maintain a priority queue of entries by expiration time,
  // to make this scanning faster.  However, we don't expect the number of
  // neighbors to be very large, so the extra complexity isn't worth
  // implementing right now.

  int count = 0;
  for (auto& portEntry : neighborsByPort) {
    // Advance the iterator manually to avoid make sure we don't
    // have invalid iterators to erased elements.
    auto& map = portEntry.second;
    auto it = map.begin();
    while (it != map.end()) {
      auto current = it;
      ++it;
      if (current->second.isExpired(now)) {
        map.erase(current);
      } else {
        count++;
      }
    }
  }

  return count;
}
} // namespace facebook::fboss
