// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

package "facebook.com/fboss/fsdb"

namespace cpp2 facebook.fboss.fsdb

cpp_include "folly/container/F14Map.h"

include "thrift/annotation/cpp.thrift"
include "thrift/annotation/thrift.thrift"

enum TestEnum {
  FIRST = 1,
  SECOND = 2,
  THIRD = 3,
}

enum TestEnum2 {
  FIRST = 1,
  SECOND = 2,
}

typedef list<TestEnum2> ListTypedef

@thrift.DeprecatedUnvalidatedAnnotations{items = {"thriftpath.root": "1"}}
struct TestStructSimple {
  1: i32 min;
  2: i32 max;
  3: optional TestEnum optionalEnum;
  4: optional i32 optionalIntegral;
  5: optional string optionalString;
}
// For illustration that multiple roots can be annotated. This
// will result in path code gen for all these roots, which can
// then be leveraged in code.

union UnionSimple {
  1: i32 integral;
  2: bool boolean;
  3: string str;
}

struct OtherStruct {
  1: map<string, i32> m;
  2: list<i32> l;
  3: set<i32> s;
  4: UnionSimple u;
  5: optional i32 o;
}

@thrift.DeprecatedUnvalidatedAnnotations{items = {"allow_skip_thrift_cow": "1"}}
struct TestHybridStruct {
  1: optional i32 optionalIntegral;
  2: string str;
  3: optional TestEnum optionalEnum;
  6: set<i32> integralSet = [];
  4: map<i32, TestStructSimple> structMap = {};
  5: list<TestStructSimple> structList = [];
}

@thrift.DeprecatedUnvalidatedAnnotations{items = {"thriftpath.root": "1"}}
struct TestStruct {
  1: bool tx = false;
  2: bool rx = false;
  3: string name;
  4: TestStructSimple member;
  @cpp.Type{template = "folly::F14FastMap"}
  @thrift.DeprecatedUnvalidatedAnnotations{
    items = {"allow_skip_thrift_cow": "1"},
  }
  5: map<i32, TestStructSimple> structMap = {};
  6: optional string optionalString;
  7: UnionSimple variantMember;
  8: list<TestStructSimple> structList = [];
  9: map<TestEnum, TestStructSimple> enumMap = {};
  10: TestEnum enumeration = TestEnum.FIRST;
  11: set<TestEnum> enumSet = [];
  12: set<i32> integralSet = [];
  @thrift.DeprecatedUnvalidatedAnnotations{
    items = {"allow_skip_thrift_cow": "1"},
  }
  13: map<string, i32> mapOfStringToI32;
  14: list<i32> listOfPrimitives;
  15: set<i32> setOfI32;
  16: map<string, TestStructSimple> stringToStruct = {};
  17: ListTypedef listTypedef = [];
  18: map<string, OtherStruct> mapOfStructs;
  19: list<OtherStruct> listofStructs;
  20: set<string> setOfStrings = [];
  21: map<i32, TestHybridStruct> mapOfHybridStruct;
}
