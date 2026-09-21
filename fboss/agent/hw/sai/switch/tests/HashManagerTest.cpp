// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/switch/SaiHashManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

class HashManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::BLANK;
    ManagerTestBase::SetUp();
  }

  cfg::Fields makeHashFields(
      std::set<cfg::IPv4Field> ipv4Fields = {},
      std::set<cfg::IPv6Field> ipv6Fields = {},
      std::set<cfg::TransportField> transportFields = {}) {
    cfg::Fields fields;
    fields.ipv4Fields() = std::move(ipv4Fields);
    fields.ipv6Fields() = std::move(ipv6Fields);
    fields.transportFields() = std::move(transportFields);
    return fields;
  }
};

TEST_F(HashManagerTest, createHashWithIpv4Fields) {
  auto fields = makeHashFields(
      {cfg::IPv4Field::SOURCE_ADDRESS, cfg::IPv4Field::DESTINATION_ADDRESS});
  auto hash = saiManagerTable->hashManager().getOrCreate(fields);
  ASSERT_NE(hash, nullptr);

  auto gotFields = saiApiTable->hashApi().getAttribute(
      hash->adapterKey(), SaiHashTraits::Attributes::NativeHashFieldList{});
  ASSERT_EQ(gotFields.size(), 2);
  EXPECT_EQ(gotFields[0], SAI_NATIVE_HASH_FIELD_SRC_IP);
  EXPECT_EQ(gotFields[1], SAI_NATIVE_HASH_FIELD_DST_IP);
}

TEST_F(HashManagerTest, createHashWithIpv6Fields) {
  auto fields = makeHashFields(
      {},
      {cfg::IPv6Field::SOURCE_ADDRESS, cfg::IPv6Field::DESTINATION_ADDRESS});
  auto hash = saiManagerTable->hashManager().getOrCreate(fields);
  ASSERT_NE(hash, nullptr);

  auto gotFields = saiApiTable->hashApi().getAttribute(
      hash->adapterKey(), SaiHashTraits::Attributes::NativeHashFieldList{});
  ASSERT_EQ(gotFields.size(), 2);
  EXPECT_EQ(gotFields[0], SAI_NATIVE_HASH_FIELD_SRC_IP);
  EXPECT_EQ(gotFields[1], SAI_NATIVE_HASH_FIELD_DST_IP);
}

TEST_F(HashManagerTest, createHashWithIpv6FlowLabel) {
  auto fields = makeHashFields(
      {},
      {cfg::IPv6Field::SOURCE_ADDRESS,
       cfg::IPv6Field::DESTINATION_ADDRESS,
       cfg::IPv6Field::FLOW_LABEL});
  auto hash = saiManagerTable->hashManager().getOrCreate(fields);
  ASSERT_NE(hash, nullptr);

  auto gotFields = saiApiTable->hashApi().getAttribute(
      hash->adapterKey(), SaiHashTraits::Attributes::NativeHashFieldList{});
  ASSERT_EQ(gotFields.size(), 3);

  // Verify flow label is included in the native hash fields
  bool hasFlowLabel = false;
  bool hasSrcIp = false;
  bool hasDstIp = false;
  for (auto field : gotFields) {
    if (field == SAI_NATIVE_HASH_FIELD_IPV6_FLOW_LABEL) {
      hasFlowLabel = true;
    } else if (field == SAI_NATIVE_HASH_FIELD_SRC_IP) {
      hasSrcIp = true;
    } else if (field == SAI_NATIVE_HASH_FIELD_DST_IP) {
      hasDstIp = true;
    }
  }
  EXPECT_TRUE(hasFlowLabel);
  EXPECT_TRUE(hasSrcIp);
  EXPECT_TRUE(hasDstIp);
}

TEST_F(HashManagerTest, createHashWithTransportFields) {
  auto fields = makeHashFields(
      {cfg::IPv4Field::SOURCE_ADDRESS, cfg::IPv4Field::DESTINATION_ADDRESS},
      {},
      {cfg::TransportField::SOURCE_PORT,
       cfg::TransportField::DESTINATION_PORT});
  auto hash = saiManagerTable->hashManager().getOrCreate(fields);
  ASSERT_NE(hash, nullptr);

  auto gotFields = saiApiTable->hashApi().getAttribute(
      hash->adapterKey(), SaiHashTraits::Attributes::NativeHashFieldList{});
  ASSERT_EQ(gotFields.size(), 4);
}

TEST_F(HashManagerTest, createFullHashWithFlowLabel) {
  // Simulates the SRv6 load balancing hash config:
  // IPv4 src/dst + IPv6 src/dst/flow_label + L4 src/dst ports
  auto fields = makeHashFields(
      {cfg::IPv4Field::SOURCE_ADDRESS, cfg::IPv4Field::DESTINATION_ADDRESS},
      {cfg::IPv6Field::SOURCE_ADDRESS,
       cfg::IPv6Field::DESTINATION_ADDRESS,
       cfg::IPv6Field::FLOW_LABEL},
      {cfg::TransportField::SOURCE_PORT,
       cfg::TransportField::DESTINATION_PORT});
  auto hash = saiManagerTable->hashManager().getOrCreate(fields);
  ASSERT_NE(hash, nullptr);

  auto gotFields = saiApiTable->hashApi().getAttribute(
      hash->adapterKey(), SaiHashTraits::Attributes::NativeHashFieldList{});
  // 2 IPv4 + 3 IPv6 + 2 transport = 7
  // But SRC_IP/DST_IP are shared between v4 and v6, so dedup may apply.
  // In practice, the current implementation adds them separately, resulting
  // in duplicate entries. The SAI adapter handles deduplication.
  ASSERT_EQ(gotFields.size(), 7);

  bool hasFlowLabel = false;
  for (auto field : gotFields) {
    if (field == SAI_NATIVE_HASH_FIELD_IPV6_FLOW_LABEL) {
      hasFlowLabel = true;
    }
  }
  EXPECT_TRUE(hasFlowLabel);
}

TEST_F(HashManagerTest, createHashReusesExisting) {
  auto fields = makeHashFields(
      {},
      {cfg::IPv6Field::SOURCE_ADDRESS,
       cfg::IPv6Field::DESTINATION_ADDRESS,
       cfg::IPv6Field::FLOW_LABEL});
  auto hash1 = saiManagerTable->hashManager().getOrCreate(fields);
  auto hash2 = saiManagerTable->hashManager().getOrCreate(fields);
  EXPECT_EQ(hash1->adapterKey(), hash2->adapterKey());
}
