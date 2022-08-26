// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

namespace cpp2 facebook.fboss.fsdb

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

struct TestStructSimple {
  1: i32 min;
  2: i32 max;
} (thriftpath.root)
// For illustration that multiple roots can be annotated. This
// will result in path code gen for all these roots, which can
// then be leveraged in code.

union UnionSimple {
  1: i32 integral;
  2: bool boolean;
  3: string str;
}

struct TestStruct {
  1: bool tx = false;
  2: bool rx = false;
  3: string name;
  4: TestStructSimple member;
  5: map<i32, TestStructSimple> structMap = {};
  6: optional string optionalString;
  7: UnionSimple variantMember;
  8: list<TestStructSimple> structList = [];
  9: map<TestEnum, TestStructSimple> enumMap = {};
  10: TestEnum enumeration = TestEnum.FIRST;
  11: set<TestEnum> enumSet = [];
  12: set<i32> integralSet = [];
  13: map<string, i32> mapOfStringToI32;
  14: list<i32> listOfPrimitives;
  15: set<i32> setOfI32;
  16: map<string, TestStructSimple> stringToStruct = {};
  17: ListTypedef listTypedef = [];
} (thriftpath.root)
