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
  2: map<string, i32> strMap;
  3: map<string, switch_config.L4PortRange> structMap;
  4: list<switch_config.L4PortRange> listOfStruct;
  5: i32 leafI32;
  6: optional TestEnum optionalEnum;
  7: set<string> childSet;
}

struct TestStruct {
  1: bool inlineBool;
  2: i32 inlineInt;
  3: string inlineString;
  4: switch_config.L4PortRange inlineStruct;
  5: optional i32 optionalInt;
  6: optional switch_config.L4PortRange optionalStruct;
  7: list<i32> listOfPrimitives (allow_skip_thrift_cow = true);
  8: list<switch_config.L4PortRange> listOfStructs;
  9: list<list<i32>> listOfListOfPrimitives;
  10: list<list<switch_config.L4PortRange>> listOfListOfStructs;
  11: map<i32, i32> mapOfI32ToI32 (allow_skip_thrift_cow = true);
  12: map<TestEnum, i32> mapOfEnumToI32 (allow_skip_thrift_cow = true);
  13: map<string, i32> mapOfStringToI32 (allow_skip_thrift_cow = true);
  14: map<i32, switch_config.L4PortRange> mapOfI32ToStruct (
    allow_skip_thrift_cow = true,
  );
  15: map<TestEnum, switch_config.L4PortRange> mapOfEnumToStruct (
    allow_skip_thrift_cow = true,
  );
  16: map<string, switch_config.L4PortRange> mapOfStringToStruct;
  17: map<i32, list<switch_config.L4PortRange>> mapOfI32ToListOfStructs (
    allow_skip_thrift_cow = true,
  );
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
  27: map<i32, bool> hybridMap (allow_skip_thrift_cow = true);
  28: list<i32> hybridList (allow_skip_thrift_cow = true);
  29: set<i32> hybridSet (allow_skip_thrift_cow = true);
  30: TestUnion hybridUnion (allow_skip_thrift_cow = true);
  31: ChildStruct hybridStruct (allow_skip_thrift_cow = true);
  // hybridMapOfI32ToStruct is crafted to cover deeper accesses inside HybridNode, with
  // paths that terminate at primitive leaves, intermediate containers (list, map, struct)
  // in UTs for various visitors: PathVisitor, RecurseVisitor, DeltaVisitor
  32: map<i32, ChildStruct> hybridMapOfI32ToStruct (
    allow_skip_thrift_cow = true,
  );
  33: map<i32, map<i32, i32>> hybridMapOfMap (allow_skip_thrift_cow = true);
  34: map<i32, set<string>> mapOfI32ToSetOfString (
    allow_skip_thrift_cow = true,
  );
}

// structs declared to mimic deeper Thrift path accesses
struct ParentTestStruct {
  1: TestStruct childStruct;
  2: map<i32, map<string, TestStruct>> mapOfI32ToMapOfStruct;
}

struct RootTestStruct {
  1: map<i32, map<string, ParentTestStruct>> mapOfI32ToMapOfStruct;
  2: list<ParentTestStruct> listOfStruct;
  3: ParentTestStruct inlineStruct;
}

// structs declared exclusively for testing Thrift annotations.
struct TestStructForAnnotation1 {
  1: TestStruct childStruct;
} (random_annotation)

struct TestStruct2 {
  10: i32 deprecatedField (deprecated);
} (deprecated, allow_skip_thrift_cow = false)

struct TestStruct3 {
  1: i32 inlineInt;
} (allow_skip_thrift_cow)
