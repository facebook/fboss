// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/tests/gen-cpp2-thriftpath/thriftpath_test.h" // @manual=//fboss/fsdb/tests:thriftpath_test_thrift-cpp2-thriftpath

namespace {
constexpr int kDefaultMapSize = 1 * 1000;
}

namespace facebook::fboss::test_data {

using facebook::fboss::fsdb::OperProtocol;
using facebook::fboss::fsdb::OperState;
using facebook::fboss::fsdb::OtherStruct;
using facebook::fboss::fsdb::TaggedOperState;
using facebook::fboss::fsdb::TestEnum;
using facebook::fboss::fsdb::TestStruct;

enum RoleSelector {
  Minimal = 0,
  MaxScale = 1,
};

class IDataGenerator {
 public:
  virtual ~IDataGenerator() = default;

  virtual TaggedOperState getStateUpdate(int version, bool minimal) = 0;
};

class TestDataFactory : public IDataGenerator {
 public:
  using RootT = TestStruct;

  explicit TestDataFactory(
      RoleSelector selector,
      int scaleFactor = kDefaultMapSize)
      : selector_(selector), scaleFactor_(scaleFactor) {}

  TaggedOperState getStateUpdate(int version, bool minimal) override;

 protected:
  OtherStruct
  buildMinimalTestData(int version, int key, std::vector<std::string>& path);

  TestStruct buildTestData(int version, std::vector<std::string>& /* path */);

  RoleSelector selector_;
  int scaleFactor_;
  OperProtocol protocol_{OperProtocol::COMPACT};
};

} // namespace facebook::fboss::test_data
