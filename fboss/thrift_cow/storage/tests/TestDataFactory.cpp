// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/thrift_cow/storage/tests/TestDataFactory.h"

#include <fmt/format.h>
#include "fboss/thrift_cow/nodes/Serializer.h"

namespace facebook::fboss::test_data {

TaggedOperState TestDataFactory::getStateUpdate(int version, bool minimal) {
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

OtherStruct TestDataFactory::buildMinimalTestData(
    int version,
    int key,
    std::vector<std::string>& path) {
  OtherStruct val;

  val.o() = key;
  val.m()[fmt::format("m.key{}", key)] = 900 + version;

  path.emplace_back("mapOfStructs");
  path.emplace_back(fmt::format("key{}", key));

  return val;
}

TestStruct TestDataFactory::buildTestData(
    int version,
    std::vector<std::string>& /* path */) {
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

} // namespace facebook::fboss::test_data
