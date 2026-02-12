// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/client/FsdbSyncManager.h"
#include "fboss/fsdb/if/FsdbModel.h"
#include "neteng/fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"

namespace bgp_thrift = facebook::neteng::fboss::bgp::thrift;

namespace facebook::fboss::fsdb::test {

/**
 * BgpRibTestPublisher - Test utility for publishing BGP RIB data to FSDB
 *
 * This class is used for testing FSDB subscribers (like HostReachTracker)
 * that consume BGP RIB updates. It supports:
 * - Publishing routes to FSDB
 * - Withdrawing routes (removing from ribMap)
 * - Signal-based orchestration for controlled testing
 *
 * Usage:
 *   BgpRibTestPublisher publisher;
 *   publisher.start();
 *   publisher.publishRoute("2001:db8::/32", entry);
 *   // ... test subscriber behavior ...
 *   publisher.withdrawRoute("2001:db8::/32");
 *   // ... test withdrawal handling ...
 *   publisher.stop();
 */
class BgpRibTestPublisher {
 public:
  explicit BgpRibTestPublisher(
      const std::string& publisherId = "bgp-rib-test-publisher");
  ~BgpRibTestPublisher();

  // Lifecycle
  void start();
  void stop();

  // Full RIB sync (initial publish)
  void publishRibMap(
      const std::map<std::string, bgp_thrift::TRibEntry>& ribMap);

  // Delta updates (add/update routes)
  void publishRoute(
      const std::string& prefix,
      const bgp_thrift::TRibEntry& entry);

  // Withdraw route - removes from ribMap, triggering subscriber withdrawal
  void withdrawRoute(const std::string& prefix);

  // Withdraw all published routes
  void withdrawAllRoutes();

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
};

} // namespace facebook::fboss::fsdb::test
