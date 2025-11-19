// Copyright 2004-present Facebook. All Rights Reserved.
#pragma once

#include <thrift/lib/cpp2/TypeClass.h>
#include "fboss/agent/lldp/LinkNeighbor.h"
#include "fboss/agent/lldp/gen-cpp2/lldp_fatal_types.h"
#include "fboss/agent/lldp/gen-cpp2/lldp_types.h"
#include "fboss/agent/types.h"
#include "fboss/thrift_cow/storage/CowStorage.h"
#include "folly/Synchronized.h"

#include <chrono>
#include <map>
#include <mutex>
#include <vector>

namespace facebook::fboss {

class LinkNeighbor;

/*
 * LinkNeighborDB maintains information about known neighbors.
 *
 * This class is thread-safe, and performs synchronization internally.
 */
class LinkNeighborDB {
 public:
  LinkNeighborDB();
  ~LinkNeighborDB() = default;

  /*
   * Update the DB with new neighbor information.
   */
  void update(std::shared_ptr<LinkNeighbor> neighbor);

  /*
   * Get all known neighbors.
   *
   * This returns a new copy of the neighbor information.
   */
  std::vector<std::shared_ptr<LinkNeighbor>> getNeighbors();

  /*
   * Get all known neighbors on a specific port.
   *
   * This returns a new copy of the neighbor information.
   */
  std::vector<std::shared_ptr<LinkNeighbor>> getNeighbors(PortID port);

  /*
   * Remove expired neighbor entries from the database and return number
   * of entries left.
   */
  int pruneExpiredNeighbors();
  int pruneExpiredNeighbors(std::chrono::steady_clock::time_point now);

  void portDown(PortID port);

 private:
  using LldpState = fsdb::CowStorage<lldp::LldpState>;

  // Returns number of entries left after pruning
  int pruneLocked(LldpState& state, std::chrono::steady_clock::time_point now);

  // Forbidden copy constructor and assignment operator
  LinkNeighborDB(LinkNeighborDB const&) = delete;
  LinkNeighborDB& operator=(LinkNeighborDB const&) = delete;
  LinkNeighborDB(LinkNeighborDB&&) = delete;
  LinkNeighborDB& operator=(LinkNeighborDB&&) = delete;

  folly::Synchronized<LldpState> byLocalPort_{LldpState(lldp::LldpState())};
};

} // namespace facebook::fboss
