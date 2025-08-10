// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/tests/gen-cpp2-thriftpath/thriftpath_test.h" // @manual=//fboss/fsdb/tests:thriftpath_test_thrift-cpp2-thriftpath
#include "fboss/thrift_cow/nodes/Serializer.h"

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

  TaggedOperState getStateUpdate(int version, bool minimal) override {
    TaggedOperState state;
    OperState chunk;
    std::vector<std::string> basePath;
    chunk.protocol() = protocol_;
    if (minimal) {
      int key = 42;
      auto data = buildMinimalTestData(version, key, basePath);
      chunk.contents() = facebook::fboss::thrift_cow::serialize<
          apache::thrift::type_class::structure>(protocol_, data);
      CHECK_EQ(basePath.size(), 2);
    } else {
      auto data = buildTestData(version, basePath);
      chunk.contents() = facebook::fboss::thrift_cow::serialize<
          apache::thrift::type_class::structure>(protocol_, data);
    }
    state.state() = chunk;
    state.path()->path() = basePath;
    return state;
  }

 protected:
  OtherStruct
  buildMinimalTestData(int version, int key, std::vector<std::string>& path) {
    OtherStruct val;

    val.o() = key;
    val.m()[fmt::format("m.key{}", key)] = 900 + version;

    path.emplace_back("mapOfStructs");
    path.emplace_back(fmt::format("key{}", key));

    return val;
  }

  TestStruct buildTestData(int version, std::vector<std::string>& /* path */) {
    TestStruct val;

    val.tx() = (version % 2) ? false : true;
    val.name() = "str";
    val.optionalString() = "optionalStr";
    val.enumeration() = TestEnum::FIRST;
    val.enumSet() = {TestEnum::FIRST, TestEnum::SECOND};
    val.integralSet() = {101, 102, 103};
    val.listOfPrimitives() = {201, 202, 203};

    auto makeOtherStruct = [](int idx, int mapSize) -> OtherStruct {
      OtherStruct other;
      other.o() = idx;
      for (int i = 0; i < mapSize; i++) {
        other.m()[fmt::format("m.key{}", i)] = 900 + i;
      }
      return other;
    };

    val.listofStructs()->emplace_back(makeOtherStruct(701, 2));
    val.listofStructs()->emplace_back(makeOtherStruct(702, 2));

    if (selector_ != Minimal) {
      int outMapSize = scaleFactor_;
      constexpr int kInnerMapSize = 8;
      for (int i = 0; i < outMapSize; i++) {
        val.mapOfStructs()[fmt::format("key{}", i)] =
            makeOtherStruct(i, kInnerMapSize);
      }
    }

    return val;
  }

  RoleSelector selector_;
  int scaleFactor_;
  OperProtocol protocol_{OperProtocol::COMPACT};
};

} // namespace facebook::fboss::test_data
