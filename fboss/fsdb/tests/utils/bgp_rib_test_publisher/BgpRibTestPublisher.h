// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/client/FsdbSyncManager.h"
#include "fboss/fsdb/if/FsdbModel.h"
#include "neteng/fboss/bgp/if/gen-cpp2/bgp_route_types_types.h"
#include "neteng/fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"

namespace bgp_thrift = facebook::neteng::fboss::bgp::thrift;

namespace facebook::fboss::fsdb::test {

/**
 * BgpRibTestPublisher - Test utility for publishing the canonical BGP RIB to
 * FSDB.
 *
 * Used to test FSDB subscribers (like HostReachTracker) that consume the
 * canonical RIB. Declare the desired routes with setCanonicalRoutes(); the
 * publisher holds the TCanonicalRibState across calls so community-list indices
 * stay stable, and prefixes omitted from a later call are withdrawn.
 *
 * Usage:
 *   BgpRibTestPublisher publisher;
 *   publisher.start();
 *   publisher.setCanonicalRoutes({{"2401:db00:1::/64", {}}});
 *   // ... test subscriber behavior ...
 *   publisher.setCanonicalRoutes({}); // withdraw all
 *   publisher.stop();
 */
class BgpRibTestPublisher {
 public:
  explicit BgpRibTestPublisher(
      const std::string& publisherId = "bgp-rib-test-publisher");
  ~BgpRibTestPublisher();

  // A canonical-RIB route: a prefix and the communities on its best path, plus
  // an optional per-route next hop. An empty nextHop falls back to the shared
  // nextHop passed to setCanonicalRoutes.
  struct CanonicalRoute {
    std::string prefix; // CIDR, e.g. "2401:db00:1::/64"
    std::vector<std::pair<int32_t, int32_t>> communities; // {asn, value} pairs
    std::string nextHop; // optional; empty -> shared default
  };

  // Lifecycle
  void start();
  void stop();

  // Declares the full set of canonical-RIB routes and publishes the resulting
  // TCanonicalRibState (BgpData.canonicalRib). Stateful across calls: the
  // de-duplicated community dictionary is carried forward so a community list's
  // index stays stable while referenced and a content change lands on a fresh
  // index -- mirroring bgpd's incremental encoder, not the dense get-rib
  // rebuild. Prefixes absent from `routes` are withdrawn.
  void setCanonicalRoutes(
      const std::vector<CanonicalRoute>& routes,
      const std::string& nextHop = "2401:db00:ffff::1");

  // Get list of currently published prefixes
  const std::vector<std::string>& getPublishedPrefixes() const {
    return publishedPrefixes_;
  }

 private:
  std::unique_ptr<FsdbPubSubManager> fsdbPubSubMgr_;
  std::unique_ptr<FsdbSyncManager<BgpData, true /* EnablePatchAPIs */>>
      stateSyncer_;
  std::string publisherId_;
  std::vector<std::string> publishedPrefixes_;
  // Accumulated canonical RIB. Persisted across setCanonicalRoutes calls so the
  // community dictionary indices stay stable like the real incremental encoder.
  bgp_thrift::TCanonicalRibState canonicalState_;
};

} // namespace facebook::fboss::fsdb::test
