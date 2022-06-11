// Copyright 2004-present Facebook. All Rights Reserved.
#pragma once

#include "fboss/agent/lldp/LinkNeighbor.h"
#include "fboss/agent/types.h"
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

  /*
   * Update the DB with new neighbor information.
   */
  void update(const LinkNeighbor& neighbor);

  /*
   * Get all known neighbors.
   *
   * This returns a new copy of the neighbor information.
   */
  std::vector<LinkNeighbor> getNeighbors();

  /*
   * Get all known neighbors on a specific port.
   *
   * This returns a new copy of the neighbor information.
   */
  std::vector<LinkNeighbor> getNeighbors(PortID port);

  /*
   * Remove expired neighbor entries from the database and return number
   * of entries left.
   */
  int pruneExpiredNeighbors();
  int pruneExpiredNeighbors(std::chrono::steady_clock::time_point now);

  void portDown(PortID port);

 private:
  class NeighborKey {
   public:
    explicit NeighborKey(const LinkNeighbor& neighbor);
    bool operator<(const NeighborKey& other) const;
    bool operator==(const NeighborKey& other) const;

   private:
    lldp::LldpChassisIdType chassisIdType_;
    lldp::LldpPortIdType portIdType_;
    std::string chassisId_;
    std::string portId_;
  };
  using NeighborMap = std::map<NeighborKey, LinkNeighbor>;
  using NeighborsByPort = std::map<PortID, NeighborMap>;

  // Returns number of entries left after pruning
  int pruneLocked(
      NeighborsByPort& neighborsByPort,
      std::chrono::steady_clock::time_point now);

  // Forbidden copy constructor and assignment operator
  LinkNeighborDB(LinkNeighborDB const&) = delete;
  LinkNeighborDB& operator=(LinkNeighborDB const&) = delete;

  folly::Synchronized<NeighborsByPort> byLocalPort_;
};

} // namespace facebook::fboss
