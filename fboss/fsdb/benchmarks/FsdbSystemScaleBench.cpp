// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/Benchmark.h>

#include "fboss/fsdb/benchmarks/FsdbBenchmarkTestHelper.h"
#include "fboss/fsdb/tests/utils/FsdbTestServer.h"
#include "fboss/lib/CommonFileUtils.h"

DEFINE_string(
    write_agent_config_marker_for_fsdb,
    "",
    "Write marker file for FSDB");

DEFINE_string(
    service_file_name_for_scale,
    "",
    "Service file name for SAI Agent Scale Benchmarks");
using facebook::fboss::fsdb::test::FsdbTestServer;

namespace {

const thriftpath::RootThriftPath<facebook::fboss::fsdb::FsdbOperStateRoot>
    stateRoot;
const std::vector<std::string> kPublishRoot{"agent"};

} // anonymous namespace

namespace facebook::fboss::fsdb::test {

BENCHMARK(FsdbSystemScaleStats) {
  folly::BenchmarkSuspender suspender;
  auto filePath =
      folly::to<std::string>(FLAGS_write_agent_config_marker_for_fsdb);
  if (FLAGS_service_file_name_for_scale.empty()) {
    return;
  }
  auto serviceFileName =
      folly::to<std::string>(FLAGS_service_file_name_for_scale);
  auto port = FLAGS_fsdbPort;
  FsdbBenchmarkTestHelper helper;
  helper.setup(0, false, serviceFileName);

  std::unique_ptr<fsdb::test::FsdbTestServer> fsdbTestServer_;
  while (true) {
    if (checkFileExists(filePath)) {
      break;
    } else {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  suspender.dismiss();
  {
    fsdbTestServer_ = std::make_unique<FsdbTestServer>(port);
    while (true) {
      auto md = fsdbTestServer_->getPublisherRootMetadata(
          *kPublishRoot.begin(), false);
      if (md.has_value()) {
        break;
      } else {
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
    }
  }
  suspender.rehire();
  {
    fsdbTestServer_.reset();
    removeFile(filePath, true);
  }
  helper.TearDown(false /*stopFsdbTestServer*/);
}
} // namespace facebook::fboss::fsdb::test
