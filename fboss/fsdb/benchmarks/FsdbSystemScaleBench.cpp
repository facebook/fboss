// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/Benchmark.h>
#include <glog/logging.h>

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
    // Original timed measurement (server creation + state publisher connect);
    // kept as-is so the timing metric stays comparable to historical values.
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

  // Wait (untimed) until the agent has actually published its scaled state and
  // stats -- not merely connected (has_value() above is true on connect) --
  // otherwise max_rss races teardown and can measure an idle server. Untimed so
  // the agent-dependent publish time does not skew the timing metric; max_rss
  // is process-wide peak RSS so it is still captured. CHECK-fail if nothing is
  // published.
  auto publishConfirmed = [&](bool isStats) {
    auto md = fsdbTestServer_->getPublisherRootMetadata(
        *kPublishRoot.begin(), isStats);
    return md.has_value() && md->operMetadata.lastConfirmedAt().value_or(0) > 0;
  };
  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(120);
  while (!(publishConfirmed(false) && publishConfirmed(true))) {
    CHECK(std::chrono::steady_clock::now() < deadline)
        << "agent did not publish scaled state and stats in time";
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  fsdbTestServer_.reset();
  removeFile(filePath, true);
  helper.TearDown(false /*stopFsdbTestServer*/);
}
} // namespace facebook::fboss::fsdb::test
