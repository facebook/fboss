#pragma once

#include <folly/synchronization/Baton.h>
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/common/Flags.h"
#include "gflags/gflags.h"

DECLARE_bool(fsdb_publish_test);
namespace facebook::fboss {

// helper class to run Agent benchmark test in Agent-FSDB integration benchmark
// tests.
// for enabling Agent-FSDB integration benchmark To publish to FSDB and
// wait for completion, this helper creates a test subscription for agent state
// and waits for subscription to be served by FSDB as indirect indication that
// publish is complete.
class AgentFsdbIntegrationBenchmarkHelper {
 public:
  AgentFsdbIntegrationBenchmarkHelper();

  void awaitCompletion(AgentEnsemble* ensemble);

  void publishCompletionMarker(AgentEnsemble* ensemble);

  ~AgentFsdbIntegrationBenchmarkHelper();

 private:
  std::unique_ptr<fsdb::FsdbPubSubManager> pubsubMgr_;
  folly::Baton<> subscriptionConnected_;
  // helper flags derived from test gflag(s)
  bool enablePublishToFsdb_{false};
  bool connectToFsdb_{false};
  bool waitForPublishConfirmed_{false};
  bool waitForDummyDataPublish_{false};
  folly::Baton<> dummyDataPublished_;
  uint64_t subscribe_latency_{0};
};

} // namespace facebook::fboss
