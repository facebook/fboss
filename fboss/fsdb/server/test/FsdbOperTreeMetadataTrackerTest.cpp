// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/fsdb/server/FsdbOperTreeMetadataTracker.h"
#include <gtest/gtest.h>

namespace facebook::fboss::fsdb::test {
namespace {
auto constexpr kPublisherId = "publisher";
const std::vector<std::string> kPublishRoot{"agent"};
} // namespace
class PublisherTreeMetadataTrackerTest : public ::testing::Test {
 protected:
  FsdbOperTreeMetadata getMetadata(
      const std::vector<std::string>& path = kPublishRoot) const {
    auto metadata = metadataTracker_.getPublisherRootMetadata(path);
    if (!metadata) {
      throw std::runtime_error(
          "Publisher root " + *path.begin() + " metadata not found");
    }
    return *metadata;
  }
  FsdbOperTreeMetadataTracker metadataTracker_;
};

TEST_F(PublisherTreeMetadataTrackerTest, registerUnregisterPublishers) {
  metadataTracker_.registerPublisherRoot(kPublishRoot);
  metadataTracker_.registerPublisherRoot(kPublishRoot);
  EXPECT_EQ(getMetadata().numOpenConnections, 2);
  metadataTracker_.unregisterPublisherRoot(kPublishRoot);
  EXPECT_EQ(getMetadata().numOpenConnections, 1);
  metadataTracker_.unregisterPublisherRoot(kPublishRoot);
  EXPECT_THROW(getMetadata(), std::runtime_error);
}

TEST_F(PublisherTreeMetadataTrackerTest, updateMetadataUnregisteredPublisher) {
  EXPECT_THROW(
      metadataTracker_.updateMetadata(kPublishRoot, {}), std::runtime_error);
}

TEST_F(PublisherTreeMetadataTrackerTest, updateMetadata) {
  metadataTracker_.registerPublisherRoot(kPublishRoot);
  metadataTracker_.registerPublisherRoot(kPublishRoot);
  OperMetadata metadata;
  // Update lastConfirmedAtSecsSinceEpoch
  metadata.lastConfirmedAtSecsSinceEpoch() = 10;
  metadataTracker_.updateMetadata(kPublishRoot, metadata);
  EXPECT_EQ(getMetadata().numOpenConnections, 2);
  EXPECT_EQ(*getMetadata().operMetadata.lastConfirmedAtSecsSinceEpoch(), 10);
  // No movement back
  metadata.lastConfirmedAtSecsSinceEpoch() = 9;
  metadataTracker_.updateMetadata(kPublishRoot, metadata);
  EXPECT_EQ(*getMetadata().operMetadata.lastConfirmedAtSecsSinceEpoch(), 10);
  EXPECT_EQ(getMetadata().numOpenConnections, 2);
}

TEST_F(PublisherTreeMetadataTrackerTest, updateMetadataMoveback) {
  metadataTracker_.registerPublisherRoot(kPublishRoot);
  metadataTracker_.registerPublisherRoot(kPublishRoot);
  OperMetadata metadata;
  // Update lastConfirmedAtSecsSinceEpoch
  metadata.lastConfirmedAtSecsSinceEpoch() = 10;
  metadataTracker_.updateMetadata(kPublishRoot, metadata);
  EXPECT_EQ(getMetadata().numOpenConnections, 2);
  // Move back
  metadata.lastConfirmedAtSecsSinceEpoch() = 9;
  metadataTracker_.updateMetadata(kPublishRoot, metadata, false);
  EXPECT_EQ(*getMetadata().operMetadata.lastConfirmedAtSecsSinceEpoch(), 9);
}
} // namespace facebook::fboss::fsdb::test
