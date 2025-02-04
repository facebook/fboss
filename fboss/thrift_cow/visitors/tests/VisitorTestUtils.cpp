// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/thrift_cow/visitors/tests/VisitorTestUtils.h"
#include <folly/json/dynamic.h>
#include <thrift/lib/cpp2/folly_dynamic/folly_dynamic.h>

namespace facebook::fboss::thrift_cow {

TestStruct createSimpleTestStruct() {
  using folly::dynamic;
  dynamic testDyn = dynamic::object("inlineBool", true)("inlineInt", 54)(
      "inlineString", "testname")("optionalString", "bla")(
      "inlineStruct", dynamic::object("min", 10)("max", 20))(
      "inlineVariant", dynamic::object("inlineInt", 99))(
      "mapOfEnumToStruct",
      dynamic::object("3", dynamic::object("min", 100)("max", 200)))(
      "mapOfI32ToI32", dynamic::object(1, 1))(
      "cowMap", dynamic::object(1, true))(
      "hybridMap", dynamic::object(1, true))(
      "hybridMapOfI32ToStruct",
      dynamic::object(
          20,
          dynamic::object("childMap", dynamic::object(2, true))("leafI32", 0)(
              "listOfStruct",
              dynamic::array(dynamic::object(), dynamic::object()))(
              "strMap", dynamic::object())(
              "structMap",
              dynamic::object("30", dynamic::object("min", 100)("max", 200)))(
              "childSet", dynamic::array("test1", "test2"))))(
      "hybridMapOfMap", dynamic::object(10, dynamic::object(20, 30)))(
      "hybridStruct",
      dynamic::object(
          "childMap", dynamic::object(10, true)(20, false)(50, false))(
          "leafI32", 0)("listOfStruct", dynamic::array())(
          "strMap", dynamic::object())("structMap", dynamic::object()))(
      "mapOfStringToI32", dynamic::object("test1", 1)("test2", 2))(
      "mapOfI32ToStruct",
      dynamic::object(20, dynamic::object("min", 400)("max", 600)))(
      "mapOfI32ToListOfStructs",
      dynamic::object(
          20, dynamic::array(dynamic::object("min", 100)("max", 200))))(
      "mapOfI32ToSetOfString",
      dynamic::object(20, dynamic::array("test1", "test2")));

  return facebook::thrift::from_dynamic<TestStruct>(
      testDyn, facebook::thrift::dynamic_format::JSON_1);
}

} // namespace facebook::fboss::thrift_cow
