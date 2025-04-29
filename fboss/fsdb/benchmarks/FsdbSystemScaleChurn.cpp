
// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/Benchmark.h>

#include "fboss/fsdb/benchmarks/FsdbBenchmarkTestHelper.h"
#include "fboss/fsdb/tests/utils/FsdbTestServer.h"
#include "fboss/lib/CommonFileUtils.h"

using facebook::fboss::fsdb::test::FsdbTestServer;
DECLARE_string(write_agent_config_marker_for_fsdb);
DEFINE_string(
    service_file_name_for_churn,
    "",
    "Service file name for SAI Agent Scale Benchmarks");
namespace {

const thriftpath::RootThriftPath<facebook::fboss::fsdb::FsdbOperStateRoot>
    stateRoot;
const std::vector<std::string> kPublishRoot{"agent"};

} // anonymous namespace

namespace facebook::fboss::fsdb::test {

BENCHMARK(FsdbSystemScaleChurn) {
  if (FLAGS_service_file_name_for_churn.empty()) {
    return;
  }
  auto serviceFileName =
      folly::to<std::string>(FLAGS_service_file_name_for_churn);

  FsdbBenchmarkTestHelper helper;
  helper.setup(0, false, serviceFileName);

  auto churn_pubsub_complete_marker =
      folly::to<std::string>(FLAGS_write_agent_config_marker_for_fsdb);
  auto port = FLAGS_fsdbPort;
  std::unique_ptr<fsdb::test::FsdbTestServer> fsdbTestServer_ =
      std::make_unique<FsdbTestServer>(port);
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
  helper.TearDown(false /*stopFsdbTestServer*/);
}
} // namespace facebook::fboss::fsdb::test
