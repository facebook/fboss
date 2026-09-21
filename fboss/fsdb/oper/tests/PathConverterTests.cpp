// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include <thrift/lib/cpp2/op/Get.h>

#include "fboss/fsdb/oper/ExtendedPathBuilder.h"
#include "fboss/fsdb/oper/PathConverter.h"

namespace facebook::fboss::fsdb::test {

TEST(PathConverterTests, ConvertStatePath) {
  thriftpath::RootThriftPath<FsdbOperStateRoot> root;
  auto path = root.agent().switchState().portMaps()["xyz"];

  auto idTokens =
      PathConverter<FsdbOperStateRoot>::pathToIdTokens(path.tokens());
  auto nameTokens =
      PathConverter<FsdbOperStateRoot>::pathToNameTokens(path.idTokens());
  EXPECT_EQ(idTokens, path.idTokens());
  EXPECT_EQ(nameTokens, path.tokens());

  // conversions should work even if the start type is already correct
  idTokens = PathConverter<FsdbOperStateRoot>::pathToIdTokens(path.idTokens());
  nameTokens =
      PathConverter<FsdbOperStateRoot>::pathToNameTokens(path.tokens());
  EXPECT_EQ(idTokens, path.idTokens());
  EXPECT_EQ(nameTokens, path.tokens());
}

TEST(PathConverterTests, ConvertStateExtendedPath) {
  auto extNamePath = ext_path_builder::raw("agent")
                         .raw("switchState")
                         .raw("portMaps")
                         .regex("1[0-5]")
                         .get();

  auto extIdPath =
      ext_path_builder::raw(
          apache::thrift::op::
              get_field_id_v<FsdbOperStateRoot, apache::thrift::ident::agent>)
          .raw(
              apache::thrift::op::
                  get_field_id_v<AgentData, apache::thrift::ident::switchState>)
          .raw(
              apache::thrift::op::get_field_id_v<
                  facebook::fboss::state::SwitchState,
                  apache::thrift::ident::portMaps>)
          .regex("1[0-5]")
          .get();

  auto idPathRes =
      PathConverter<FsdbOperStateRoot>::extPathToIdTokens(*extNamePath.path());
  auto namePathRes =
      PathConverter<FsdbOperStateRoot>::extPathToNameTokens(*extIdPath.path());
  EXPECT_EQ(idPathRes, *extIdPath.path());
  EXPECT_EQ(namePathRes, *extNamePath.path());

  // conversions should work even if the start type is already correct
  idPathRes =
      PathConverter<FsdbOperStateRoot>::extPathToIdTokens(*extIdPath.path());
  namePathRes = PathConverter<FsdbOperStateRoot>::extPathToNameTokens(
      *extNamePath.path());
  EXPECT_EQ(idPathRes, *extIdPath.path());
  EXPECT_EQ(namePathRes, *extNamePath.path());
}

} // namespace facebook::fboss::fsdb::test
