// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/fsdb/server/FsdbOperTreeMetadataTracker.h"
#include <gtest/gtest.h>

namespace facebook::fboss::fsdb::test {
auto constexpr kPublisherId = "publisher";
class PublisherMetadataTrackerTest : public ::testing::Test {
 protected:
  FsdbOperTreeMetadata getMetadata(
      const PublisherId& publisher = kPublisherId) const {
    auto allMetadata = metadataTracker_.getAllMetadata();
    auto itr = allMetadata.find(kPublisherId);
    if (itr == allMetadata.end()) {
      throw std::runtime_error(
          "Publisher: " + publisher + " metadata not found");
    }
    return itr->second;
  }
  FsdbOperTreeMetadataTracker metadataTracker_;
};

TEST_F(PublisherMetadataTrackerTest, registerUnregisterPublishers) {
  metadataTracker_.registerPublisher(kPublisherId);
  metadataTracker_.registerPublisher(kPublisherId);
  EXPECT_EQ(getMetadata().numOpenConnections, 2);
  metadataTracker_.unregisterPublisher(kPublisherId);
  EXPECT_EQ(getMetadata().numOpenConnections, 1);
  metadataTracker_.unregisterPublisher(kPublisherId);
  EXPECT_THROW(getMetadata(), std::runtime_error);
}

TEST_F(PublisherMetadataTrackerTest, updateMetadataUnregisteredPublisher) {
  EXPECT_THROW(
      metadataTracker_.updateMetadata(kPublisherId, {}), std::runtime_error);
}

TEST_F(PublisherMetadataTrackerTest, updateMetadata) {
  metadataTracker_.registerPublisher(kPublisherId);
  metadataTracker_.registerPublisher(kPublisherId);
  OperMetadata metadata;
  // Update lastConfirmedAtSecsSinceEpoch
  metadata.lastConfirmedAtSecsSinceEpoch() = 10;
  metadataTracker_.updateMetadata(kPublisherId, metadata);
  EXPECT_EQ(getMetadata().numOpenConnections, 2);
  EXPECT_EQ(*getMetadata().operMetadata.generation(), 0);
  EXPECT_EQ(*getMetadata().operMetadata.lastConfirmedAtSecsSinceEpoch(), 10);
  // No movement back
  metadata.lastConfirmedAtSecsSinceEpoch() = 9;
  metadataTracker_.updateMetadata(kPublisherId, metadata);
  EXPECT_EQ(*getMetadata().operMetadata.lastConfirmedAtSecsSinceEpoch(), 10);
  // Update generation numbers
  metadata.generation() = 5;
  metadataTracker_.updateMetadata(kPublisherId, metadata);
  EXPECT_EQ(getMetadata().numOpenConnections, 2);
  EXPECT_EQ(*getMetadata().operMetadata.lastConfirmedAtSecsSinceEpoch(), 10);
  EXPECT_EQ(*getMetadata().operMetadata.generation(), 5);
  // No movement back
  metadata.generation() = 4;
  metadataTracker_.updateMetadata(kPublisherId, metadata);
  EXPECT_EQ(*getMetadata().operMetadata.generation(), 5);
}

TEST_F(PublisherMetadataTrackerTest, updateMetadataMoveback) {
  metadataTracker_.registerPublisher(kPublisherId);
  metadataTracker_.registerPublisher(kPublisherId);
  OperMetadata metadata;
  // Update lastConfirmedAtSecsSinceEpoch
  metadata.lastConfirmedAtSecsSinceEpoch() = 10;
  metadataTracker_.updateMetadata(kPublisherId, metadata);
  EXPECT_EQ(getMetadata().numOpenConnections, 2);
  EXPECT_EQ(*getMetadata().operMetadata.generation(), 0);
  EXPECT_EQ(*getMetadata().operMetadata.lastConfirmedAtSecsSinceEpoch(), 10);
  // Move back
  metadata.lastConfirmedAtSecsSinceEpoch() = 9;
  metadataTracker_.updateMetadata(kPublisherId, metadata, false);
  EXPECT_EQ(*getMetadata().operMetadata.lastConfirmedAtSecsSinceEpoch(), 9);
  // Update generation numbers
  metadata.generation() = 5;
  metadataTracker_.updateMetadata(kPublisherId, metadata);
  EXPECT_EQ(getMetadata().numOpenConnections, 2);
  EXPECT_EQ(*getMetadata().operMetadata.lastConfirmedAtSecsSinceEpoch(), 9);
  EXPECT_EQ(*getMetadata().operMetadata.generation(), 5);
  // Move back
  metadata.generation() = 4;
  metadataTracker_.updateMetadata(kPublisherId, metadata, false);
  EXPECT_EQ(*getMetadata().operMetadata.generation(), 4);
}
} // namespace facebook::fboss::fsdb::test
