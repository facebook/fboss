// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/fsdb/oper/ExtendedPathBuilder.h"

namespace {

using namespace facebook::fboss::fsdb;

OperPathElem raw(std::string tok) {
  OperPathElem elem;
  elem.raw_ref() = tok;
  return elem;
}

OperPathElem regex(std::string tok) {
  OperPathElem elem;
  elem.regex_ref() = tok;
  return elem;
}

OperPathElem any() {
  OperPathElem elem;
  elem.any_ref() = true;
  return elem;
}

} // namespace

TEST(ExtendedPathBuilderTests, TestGet) {
  using namespace facebook::fboss::fsdb;

  // path builder + expected result
  std::vector<std::pair<ExtendedPathBuilder, std::vector<OperPathElem>>> tests =
      {{ext_path_builder::raw("agent").raw("switchState"),
        {raw("agent"), raw("switchState")}},
       {ext_path_builder::raw("agent").raw("switchState").regex("port.*"),
        {raw("agent"), raw("switchState"), regex("port.*")}},
       {ext_path_builder::raw("agent").raw("switchState").any(),
        {raw("agent"), raw("switchState"), any()}},
       {ext_path_builder::raw("agent").raw("switchState").raw("portMap"),
        {raw("agent"), raw("switchState"), raw("portMap")}},
       {ext_path_builder::raw("agent")
            .raw("switchState")
            .raw("portMap")
            .regex("1[0-5]"),
        {raw("agent"), raw("switchState"), raw("portMap"), regex("1[0-5]")}},
       {ext_path_builder::raw("agent").raw("switchState").raw("portMap").any(),
        {raw("agent"), raw("switchState"), raw("portMap"), any()}},
       {ext_path_builder::raw("agent").raw("switchStateNonexistent"),
        {raw("agent"), raw("switchStateNonexistent")}}};

  for (const auto& [builder, expectedPath] : tests) {
    EXPECT_THAT(*builder.get().path(), ::testing::ContainerEq(expectedPath));
  }
}
