// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/Benchmark.h>
#include <folly/init/Init.h>
#include <folly/json/dynamic.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>
#include <gtest/gtest.h>

#include "fboss/fsdb/common/Utils.h"

DECLARE_int64(bm_max_iters);

namespace facebook::fboss::fsdb::test {

BENCHMARK(fsdbClientIdParse) {
  folly::BenchmarkSuspender suspender;

  suspender.dismiss();
  for (int i = 0; i < FLAGS_bm_max_iters; ++i) {
    auto clientId = fsdb::string2FsdbClient("test_client");
    CHECK(clientId == FsdbClient::UNSPECIFIED);
  }
  suspender.rehire();
}

} // namespace facebook::fboss::fsdb::test

int main(int argc, char* argv[]) {
  folly::init(&argc, &argv);
  FLAGS_bm_max_iters = 1000;
  folly::runBenchmarks();
  return 0;
}
