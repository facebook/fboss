// Copyright 2004-present Facebook. All Rights Reserved.
#include "fboss/agent/lldp/LinkNeighborDB.h"

using std::lock_guard;
using std::vector;
using std::chrono::steady_clock;

namespace facebook::fboss {

using k = lldp::lldp_tags::strings;

LinkNeighborDB::LinkNeighborDB() {}

void LinkNeighborDB::update(std::shared_ptr<LinkNeighbor> neighbor) {
  auto neighborsLocked = byLocalPort_.wlock();
  // Go ahead and prune expired neighbors each time we get updated.
  pruneLocked(*neighborsLocked, steady_clock::now());
  auto neighborMapByPort = neighborsLocked->root()->ref<k::neighborMapByPort>();
  auto neighborMapIt = neighborMapByPort->try_emplace(neighbor->getLocalPort());
  auto neighborMap = neighborMapIt.first->second;
  auto key = neighbor->getKey();
  (*neighborMap)[key] = std::move(neighbor);
}

vector<std::shared_ptr<LinkNeighbor>> LinkNeighborDB::getNeighbors() {
  vector<std::shared_ptr<LinkNeighbor>> results;
  auto neighborsLocked = byLocalPort_.rlock();
  for (const auto& portEntry :
       *neighborsLocked->root()->ref<k::neighborMapByPort>()) {
    for (const auto& entry : *portEntry.second) {
      results.push_back(entry.second);
    }
  }

  return results;
}

vector<std::shared_ptr<LinkNeighbor>> LinkNeighborDB::getNeighbors(
    PortID port) {
  vector<std::shared_ptr<LinkNeighbor>> results;
  auto neighborsLocked = byLocalPort_.rlock();
  auto neighborMapByPort = neighborsLocked->root()->ref<k::neighborMapByPort>();
  if (auto it = neighborMapByPort->find(port); it != neighborMapByPort->end()) {
    for (const auto& entry : *it->second) {
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
      [this, now](auto& state) { return pruneLocked(state, now); });
}

void LinkNeighborDB::portDown(PortID port) {
  byLocalPort_.withWLock([port](auto& neighborsLocked) {
    // Port went down, prune lldp entries for that port
    auto neighborMapByPort =
        neighborsLocked.root()->template ref<k::neighborMapByPort>();
    neighborMapByPort->remove(port);
  });
}

int LinkNeighborDB::pruneLocked(
    LldpState& state,
    std::chrono::steady_clock::time_point now) {
  // We just do a linear scan for now.
  //
  // We could maintain a priority queue of entries by expiration time,
  // to make this scanning faster.  However, we don't expect the number of
  // neighbors to be very large, so the extra complexity isn't worth
  // implementing right now.

  int count = 0;
  auto neighborsByPort = state.root()->ref<k::neighborMapByPort>();
  for (auto& portEntry : *neighborsByPort) {
    // Advance the iterator manually to avoid make sure we don't
    // have invalid iterators to erased elements.
    auto& map = portEntry.second;
    auto it = map->begin();
    while (it != map->end()) {
      auto current = it;
      ++it;
      if (current->second->isExpired(now)) {
        map->erase(current);
      } else {
        count++;
      }
    }
  }

  return count;
}
} // namespace facebook::fboss
