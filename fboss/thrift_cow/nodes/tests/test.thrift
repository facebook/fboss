namespace cpp2 facebook.fboss

include "fboss/agent/switch_config.thrift"
include "thrift/annotation/cpp.thrift"

enum TestEnum {
  FIRST = 1,
  SECOND = 2,
  THIRD = 3,
}

union TestUnion {
  1: bool inlineBool;
  2: i32 inlineInt;
  3: string inlineString;
  4: switch_config.L4PortRange inlineStruct;
  7: list<i32> listOfPrimitives;
  8: list<switch_config.L4PortRange> listOfStructs;
  9: list<list<i32>> listOfListOfPrimitives;
  10: list<list<switch_config.L4PortRange>> listOfListOfStructs;
  11: map<i32, i32> mapOfI32ToI32;
  12: map<TestEnum, i32> mapOfEnumToI32;
  13: map<string, i32> mapOfStringToI32;
  14: map<i32, switch_config.L4PortRange> mapOfI32ToStruct;
  15: map<TestEnum, switch_config.L4PortRange> mapOfEnumToStruct;
  16: map<string, switch_config.L4PortRange> mapOfStringToStruct;
  17: map<i32, list<switch_config.L4PortRange>> mapOfI32ToListOfStructs;
  18: set<i32> setOfI32;
  19: set<TestEnum> setOfEnum;
  20: set<string> setOfString;
}

struct ChildStruct {
  1: map<i32, bool> childMap;
}

struct TestStruct {
  1: bool inlineBool;
  2: i32 inlineInt;
  3: string inlineString;
  4: switch_config.L4PortRange inlineStruct;
  5: optional i32 optionalInt;
  6: optional switch_config.L4PortRange optionalStruct;
  7: list<i32> listOfPrimitives;
  8: list<switch_config.L4PortRange> listOfStructs;
  9: list<list<i32>> listOfListOfPrimitives;
  10: list<list<switch_config.L4PortRange>> listOfListOfStructs;
  11: map<i32, i32> mapOfI32ToI32;
  12: map<TestEnum, i32> mapOfEnumToI32;
  13: map<string, i32> mapOfStringToI32;
  14: map<i32, switch_config.L4PortRange> mapOfI32ToStruct;
  15: map<TestEnum, switch_config.L4PortRange> mapOfEnumToStruct;
  16: map<string, switch_config.L4PortRange> mapOfStringToStruct;
  17: map<i32, list<switch_config.L4PortRange>> mapOfI32ToListOfStructs;
  18: set<i32> setOfI32;
  19: set<TestEnum> setOfEnum;
  20: set<string> setOfString;
  21: TestUnion inlineVariant;
  22: optional string optionalString;
  @cpp.Type{name = "uint64_t"}
  23: i64 unsigned_int64;
  24: map<string, TestStruct> mapA;
  25: map<string, TestStruct> mapB;
  26: map<i32, bool> cowMap;
  27: map<i32, bool> hybridMap; // (allow_skip_thrift_cow = true);
  28: list<i32> hybridList; // (allow_skip_thrift_cow = true);
  29: set<i32> hybridSet; // (allow_skip_thrift_cow = true);
  30: TestUnion hybridUnion; // (allow_skip_thrift_cow = true);
  31: ChildStruct hybridStruct; // (allow_skip_thrift_cow = true);
  32: map<i32, switch_config.L4PortRange> hybridMapOfI32ToStruct; // (allow_skip_thrift_cow = true)
} // (allow_skip_thrift_cow = true)

struct ParentTestStruct {
  1: TestStruct childStruct;
} (random_annotation)

struct TestStruct2 {} (deprecated, allow_skip_thrift_cow = false)
