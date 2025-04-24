
// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/Benchmark.h>

#include "fboss/fsdb/common/Flags.h"
#include "fboss/fsdb/tests/utils/FsdbTestServer.h"
#include "fboss/lib/CommonFileUtils.h"

DEFINE_string(
    write_agent_config_marker_for_fsdb,
    "",
    "Write marker file for FSDB");

using facebook::fboss::fsdb::test::FsdbTestServer;

namespace {

const thriftpath::RootThriftPath<facebook::fboss::fsdb::FsdbOperStateRoot>
    stateRoot;
const std::vector<std::string> kPublishRoot{"agent"};

} // anonymous namespace

namespace facebook::fboss::fsdb::test {

BENCHMARK(FsdbSystemScaleChurn) {
  auto churn_pubsub_complete_marker =
      folly::to<std::string>(FLAGS_write_agent_config_marker_for_fsdb);
  auto port = FLAGS_fsdbPort;
  std::unique_ptr<fsdb::test::FsdbTestServer> fsdbTestServer_ =
      std::make_unique<FsdbTestServer>(port);

  // Once publisher root is available, wait until churn is complete
  while (true) {
    if (checkFileExists(churn_pubsub_complete_marker)) {
      break;
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  // Once churn is complete, remove the marker file and exit
  fsdbTestServer_.reset();
  removeFile(churn_pubsub_complete_marker, true);
  helper.TearDown(false);
}
} // namespace facebook::fboss::fsdb::test
