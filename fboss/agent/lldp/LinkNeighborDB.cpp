// Copyright 2004-present Facebook. All Rights Reserved.
#include "fboss/agent/lldp/LinkNeighborDB.h"

using std::lock_guard;
using std::mutex;
using std::vector;
using std::chrono::steady_clock;

namespace facebook::fboss {

LinkNeighborDB::NeighborKey::NeighborKey(const LinkNeighbor& neighbor)
    : chassisIdType_(neighbor.getChassisIdType()),
      portIdType_(neighbor.getPortIdType()),
      chassisId_(neighbor.getChassisId()),
      portId_(neighbor.getPortId()) {}

bool LinkNeighborDB::NeighborKey::operator<(const NeighborKey& other) const {
  if (chassisIdType_ < other.chassisIdType_) {
    return true;
  } else if (chassisIdType_ > other.chassisIdType_) {
    return false;
  }

  if (portIdType_ < other.portIdType_) {
    return true;
  } else if (portIdType_ > other.portIdType_) {
    return false;
  }

  if (chassisId_ < other.chassisId_) {
    return true;
  } else if (chassisId_ > other.chassisId_) {
    return false;
  }

  if (portId_ < other.portId_) {
    return true;
  } else if (portId_ > other.portId_) {
    return false;
  }

  return false;
}

bool LinkNeighborDB::NeighborKey::operator==(const NeighborKey& other) const {
  return (
      chassisIdType_ == other.chassisIdType_ &&
      portIdType_ == other.portIdType_ && chassisId_ == other.chassisId_ &&
      portId_ == other.portId_);
}

LinkNeighborDB::LinkNeighborDB() {}

void LinkNeighborDB::update(const LinkNeighbor& neighbor) {
  lock_guard<mutex> guard(mutex_);

  // Go ahead and prune expired neighbors each time we get updated.
  pruneLocked(steady_clock::now());

  auto it = byLocalPort_.find(neighbor.getLocalPort());
  if (it == byLocalPort_.end()) {
    // This is the first time we have seen data for this port.
    auto ret = byLocalPort_.emplace(neighbor.getLocalPort(), NeighborMap());
    it = ret.first;
  }

  NeighborKey key(neighbor);
  // It would be nicer to use insert_or_assign() once we move to C++17
  it->second[key] = neighbor;
}

vector<LinkNeighbor> LinkNeighborDB::getNeighbors() {
  vector<LinkNeighbor> results;
  lock_guard<mutex> guard(mutex_);

  for (const auto& portEntry : byLocalPort_) {
    for (const auto& entry : portEntry.second) {
      results.push_back(entry.second);
    }
  }

  return results;
}

vector<LinkNeighbor> LinkNeighborDB::getNeighbors(PortID port) {
  vector<LinkNeighbor> results;
  lock_guard<mutex> guard(mutex_);

  auto it = byLocalPort_.find(port);
  if (it != byLocalPort_.end()) {
    for (const auto& entry : it->second) {
      results.push_back(entry.second);
    }
  }

  return results;
}

int LinkNeighborDB::pruneExpiredNeighbors() {
  lock_guard<mutex> guard(mutex_);
  return pruneLocked(steady_clock::now());
}

int LinkNeighborDB::pruneExpiredNeighbors(steady_clock::time_point now) {
  lock_guard<mutex> guard(mutex_);
  return pruneLocked(now);
}

void LinkNeighborDB::portDown(PortID port) {
  lock_guard<mutex> guard(mutex_);
  // Port went down, prune lldp entries for that port
  byLocalPort_.erase(port);
}

int LinkNeighborDB::pruneLocked(steady_clock::time_point now) {
  // We just do a linear scan for now.
  //
  // We could maintain a priority queue of entries by expiration time,
  // to make this scanning faster.  However, we don't expect the number of
  // neighbors to be very large, so the extra complexity isn't worth
  // implementing right now.

  int count = 0;
  for (auto& portEntry : byLocalPort_) {
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
