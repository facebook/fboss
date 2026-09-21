// Copyright 2004-present Facebook. All Rights Reserved.
#include "fboss/lib/TimeSeriesWithMinMax.h"

#include <folly/Benchmark.h>
#include <folly/Synchronized.h>
#include "common/init/Init.h"

#include <algorithm>
#include <vector>

using namespace facebook::fboss;
using namespace folly;

BENCHMARK(InsertionTest, n) {
  TimeSeriesWithMinMax<int> buf;
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      buf.addValue(j);
    }
    buf.getMax();
  }
}

BENCHMARK(VectorComparison, n) {
  Synchronized<std::vector<int64_t>> buf_;
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      auto buf = *(buf_.wlock());
      if (buf.size() > 60) {
        buf.push_back(j);
        buf.erase(buf.begin(), buf.begin() + 1);
      }
    }
    auto buf = *(buf_.rlock());
    std::accumulate(
        buf.begin(), buf.end(), 0, [](int x, int y) { return std::max(x, y); });
  }
}

int main(int argc, char** argv) {
  facebook::initFacebook(&argc, &argv);
  runBenchmarks();
  return 0;
}
