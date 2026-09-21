// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/if/gen-cpp2/fsdb_clients.h"
#include "fboss/lib/thrift_service_client/ConnectionOptions.h"

#include <gtest/gtest.h>

namespace facebook::fboss::utils::test {

TEST(ConnectionOptionsTests, defaultOptions) {
  auto options =
      ConnectionOptions::defaultOptions<facebook::fboss::fsdb::FsdbService>();
  EXPECT_EQ(options.getDstAddr().getAddressStr(), "::1");
  EXPECT_EQ(options.getDstAddr().getPort(), 5908);
}

TEST(ConnectionOptionsTests, tos) {
  auto options =
      ConnectionOptions::defaultOptions<facebook::fboss::fsdb::FsdbService>()
          .setTrafficClass(ConnectionOptions::TrafficClass::NC);
  EXPECT_EQ(options.getTos(), 48 << 2);
}

} // namespace facebook::fboss::utils::test
