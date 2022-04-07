// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/fsdb/server/FsdbPublisherMetadataTracker.h"
#include <gtest/gtest.h>

namespace facebook::fboss::fsdb::test {
auto constexpr kPublisherId = "publisher";
class PublisherMetadataTrackerTest : public ::testing::Test {
 protected:
  FsdbPublisherMetadataTracker metadataTracker_;
};

TEST_F(PublisherMetadataTrackerTest, registerUnregisterPublishers) {
  metadataTracker_.registerPublisher(kPublisherId);
  metadataTracker_.registerPublisher(kPublisherId);
  EXPECT_EQ(
      metadataTracker_.getAllMetadata()[kPublisherId].numOpenConnections, 2);
  metadataTracker_.unregisterPublisher(kPublisherId);
  EXPECT_EQ(
      metadataTracker_.getAllMetadata()[kPublisherId].numOpenConnections, 1);
  metadataTracker_.unregisterPublisher(kPublisherId);
  EXPECT_EQ(
      metadataTracker_.getAllMetadata()[kPublisherId].numOpenConnections, 0);
}

} // namespace facebook::fboss::fsdb::test
