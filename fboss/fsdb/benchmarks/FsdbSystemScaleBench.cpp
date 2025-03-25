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

BENCHMARK(FsdbSystemScaleStats) {
  folly::BenchmarkSuspender suspender;
  auto filePath =
      folly::to<std::string>(FLAGS_write_agent_config_marker_for_fsdb);
  auto port = FLAGS_fsdbPort;
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
}
} // namespace facebook::fboss::fsdb::test
