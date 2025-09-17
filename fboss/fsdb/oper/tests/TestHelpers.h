// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/json/dynamic.h>

#include <thrift/lib/cpp2/folly_dynamic/folly_dynamic.h>
#include "fboss/fsdb/tests/gen-cpp2-thriftpath/thriftpath_test.h" // @manual=//fboss/fsdb/tests:thriftpath_test_thrift-cpp2-thriftpath
#include "fboss/fsdb/tests/gen-cpp2/thriftpath_test_types.h"

using folly::dynamic;

namespace {

using namespace facebook::fboss::fsdb;

dynamic createTestDynamic() {
  auto dyn = dynamic::object("tx", true)("rx", false)("name", "testname")(
      "optionalString", "bla")("enumeration", 1)("enumMap", dynamic::object)(
      "member", dynamic::object("min", 10)("max", 20))(
      "variantMember", dynamic::object("integral", 99))(
      "structMap", dynamic::object(3, dynamic::object("min", 100)("max", 200)))(
      "structList", dynamic::array())("enumSet", dynamic::array())(
      "integralSet", dynamic::array())("mapOfStringToI32", dynamic::object())(
      "listOfPrimitives", dynamic::array())("setOfI32", dynamic::array());
  return dyn;
}

TestStruct initializeTestStruct() {
  dynamic dyn = createTestDynamic();
  return facebook::thrift::from_dynamic<TestStruct>(
      dyn, facebook::thrift::dynamic_format::JSON_1);
}

TestStruct createTestStructForExtendedTests() {
  auto testDyn = createTestDynamic();
  for (int i = 0; i <= 20; ++i) {
    testDyn["mapOfStringToI32"][fmt::format("test{}", i)] = i;
    testDyn["listOfPrimitives"].push_back(i);
    testDyn["setOfI32"].push_back(i);
  }

  return facebook::thrift::from_dynamic<TestStruct>(
      testDyn, facebook::thrift::dynamic_format::JSON_1);
}

} // namespace
