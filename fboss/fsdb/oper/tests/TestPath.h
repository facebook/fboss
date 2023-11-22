// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <fatal/container/tuple.h>
#include <fatal/type/pair.h>
#include <fboss/fsdb/oper/Path.h>
#include "fboss/fsdb/tests/gen-cpp2/test_fatal_types.h"
#include "fboss/fsdb/tests/gen-cpp2/test_types.h"

/*
 * This is an example of what Path classes will look like. Eventually,
 * we want to codegen this class based on the thrift IDL, but we will
 * use a handcrafted version until we have a clear idea of what we
 * want.
 */

namespace att = apache::thrift::type_class;

using k = facebook::fboss::thriftpath_test_tags::strings;

namespace {

inline std::vector<std::string> extend(
    const std::vector<std::string>& parents,
    std::string last) {
  std::vector<std::string> out(parents.size() + 1);
  std::copy(parents.begin(), parents.end(), out.begin());
  out[parents.size()] = std::move(last);
  return out;
}

} // namespace

namespace facebook::fboss::fsdb {

struct TestStruct_Path;

template <typename ParentT>
struct TestStruct_structMap_Path;

template <typename ParentT>
struct TestStructSimple_Path
    : public Path<TestStructSimple, TestStruct, att::structure, ParentT> {
  using Self = Path<TestStructSimple, TestStruct, att::structure, ParentT>;
  template <typename ChildType, typename ChildTC>
  using Child = Path<ChildType, TestStruct, ChildTC, Self>;

  explicit TestStructSimple_Path(std::vector<std::string> tokens = {})
      : Self(tokens) {}

  using Children = fatal::tuple<
      std::pair<k::min, Child<int32_t, att::integral>>,
      std::pair<k::max, Child<int32_t, att::integral>>>;

  template <typename Name>
  using TypeFor = typename Children::template type_of<Name>;

  STRUCT_CHILD_GETTERS(min)
  STRUCT_CHILD_GETTERS(max)
};

template <typename ParentT>
struct TestStruct_structMap_Path : public Path<
                                       std::map<int32_t, TestStructSimple>,
                                       TestStruct,
                                       att::map<att::integral, att::structure>,
                                       ParentT> {
  using Self = Path<
      std::map<int32_t, TestStructSimple>,
      TestStruct,
      att::map<att::integral, att::structure>,
      ParentT>;
  using Child = TestStructSimple_Path<Self>;

  explicit TestStruct_structMap_Path(std::vector<std::string> tokens = {})
      : Self(tokens) {}

  CONTAINER_CHILD_GETTERS(uint32_t)
};

template <typename ParentT>
struct TestStruct_enumMap_Path : public Path<
                                     std::map<TestEnum, TestStructSimple>,
                                     TestStruct,
                                     att::map<att::enumeration, att::structure>,
                                     ParentT> {
  using Self = Path<
      std::map<TestEnum, TestStructSimple>,
      TestStruct,
      att::map<att::enumeration, att::structure>,
      ParentT>;
  using Child = TestStructSimple_Path<Self>;

  explicit TestStruct_enumMap_Path(std::vector<std::string> tokens = {})
      : Self(tokens) {}

  CONTAINER_CHILD_GETTERS(TestEnum)
};

template <typename ParentT>
struct TestStruct_structList_Path : public Path<
                                        std::vector<TestStructSimple>,
                                        TestStruct,
                                        att::list<att::structure>,
                                        ParentT> {
  using Self = Path<
      std::vector<TestStructSimple>,
      TestStruct,
      att::list<att::structure>,
      ParentT>;

  using Child = TestStructSimple_Path<Self>;

  explicit TestStruct_structList_Path(std::vector<std::string> tokens = {})
      : Path<
            std::vector<TestStructSimple>,
            TestStruct,
            att::list<att::structure>,
            ParentT>(tokens) {}

  CONTAINER_CHILD_GETTERS(int32_t)
};

template <typename ParentT>
struct UnionSimple_Path
    : public Path<UnionSimple, TestStruct, att::variant, ParentT> {
  using Self = Path<UnionSimple, TestStruct, att::variant, ParentT>;

  template <typename ChildType, typename ChildTC>
  using Child = Path<ChildType, TestStruct, ChildTC, Self>;

  using Children = fatal::tuple<
      std::pair<k::integral, Child<int32_t, att::integral>>,
      std::pair<k::boolean, Child<bool, att::integral>>,
      std::pair<k::str, Child<std::string, att::string>>>;
  template <typename Name>
  using TypeFor = typename Children::template type_of<Name>;

  explicit UnionSimple_Path(std::vector<std::string> tokens = {})
      : Self(tokens) {}

  STRUCT_CHILD_GETTERS(integral)
  STRUCT_CHILD_GETTERS(boolean)
  STRUCT_CHILD_GETTERS(str)
};

struct TestStruct_Path
    : public Path<TestStruct, TestStruct, att::structure, folly::Unit> {
  using Self = Path<TestStruct, TestStruct, att::structure, folly::Unit>;
  template <typename ChildType, typename ChildTC>
  using Child = Path<ChildType, TestStruct, ChildTC, Self>;
  using Children = fatal::tuple<
      std::pair<k::tx, Child<bool, att::integral>>,
      std::pair<k::rx, Child<bool, att::integral>>,
      std::pair<k::name, Child<std::string, att::string>>,
      std::pair<k::member, TestStructSimple_Path<Self>>,
      std::pair<k::structMap, TestStruct_structMap_Path<Self>>,
      std::pair<k::enumMap, TestStruct_enumMap_Path<Self>>,
      std::pair<k::optionalString, Child<std::string, att::string>>,
      std::pair<k::variantMember, UnionSimple_Path<Self>>,
      std::pair<k::structList, TestStruct_structList_Path<Self>>,
      std::pair<k::enumeration, Child<TestEnum, att::enumeration>>

      >;
  template <typename Name>
  using TypeFor = typename Children::template type_of<Name>;

  explicit TestStruct_Path(std::vector<std::string> tokens = {})
      : Path<TestStruct, TestStruct, att::structure, folly::Unit>(tokens) {}

  STRUCT_CHILD_GETTERS(tx)
  STRUCT_CHILD_GETTERS(rx)
  STRUCT_CHILD_GETTERS(name)
  STRUCT_CHILD_GETTERS(member)
  STRUCT_CHILD_GETTERS(structMap)
  STRUCT_CHILD_GETTERS(enumMap)
  STRUCT_CHILD_GETTERS(optionalString)
  STRUCT_CHILD_GETTERS(variantMember)
  STRUCT_CHILD_GETTERS(structList)
  STRUCT_CHILD_GETTERS(enumeration)
};

} // namespace facebook::fboss::fsdb
