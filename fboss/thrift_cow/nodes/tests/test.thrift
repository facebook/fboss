namespace cpp2 facebook.fboss

include "fboss/agent/switch_config.thrift"
include "thrift/annotation/cpp.thrift"
include "thrift/annotation/thrift.thrift"

@thrift.AllowLegacyMissingUris
package;

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

@thrift.DeprecatedUnvalidatedAnnotations{items = {"allow_skip_thrift_cow": "1"}}
struct ChildStruct {
  1: map<i32, bool> childMap;
  2: map<string, i32> strMap;
  3: map<string, switch_config.L4PortRange> structMap;
  4: list<switch_config.L4PortRange> listOfStruct;
  5: i32 leafI32;
  6: optional TestEnum optionalEnum;
  7: set<string> childSet;
  8: optional TestUnion optionalUnion;
}

struct TestStruct {
  1: bool inlineBool;
  2: i32 inlineInt;
  3: string inlineString;
  4: switch_config.L4PortRange inlineStruct;
  5: optional i32 optionalInt;
  6: optional switch_config.L4PortRange optionalStruct;
  @thrift.DeprecatedUnvalidatedAnnotations{
    items = {"allow_skip_thrift_cow": "1"},
  }
  7: list<i32> listOfPrimitives;
  8: list<switch_config.L4PortRange> listOfStructs;
  9: list<list<i32>> listOfListOfPrimitives;
  10: list<list<switch_config.L4PortRange>> listOfListOfStructs;
  @thrift.DeprecatedUnvalidatedAnnotations{
    items = {"allow_skip_thrift_cow": "1"},
  }
  11: map<i32, i32> mapOfI32ToI32;
  @thrift.DeprecatedUnvalidatedAnnotations{
    items = {"allow_skip_thrift_cow": "1"},
  }
  12: map<TestEnum, i32> mapOfEnumToI32;
  @thrift.DeprecatedUnvalidatedAnnotations{
    items = {"allow_skip_thrift_cow": "1"},
  }
  13: map<string, i32> mapOfStringToI32;
  @thrift.DeprecatedUnvalidatedAnnotations{
    items = {"allow_skip_thrift_cow": "1"},
  }
  14: map<i32, switch_config.L4PortRange> mapOfI32ToStruct;
  @thrift.DeprecatedUnvalidatedAnnotations{
    items = {"allow_skip_thrift_cow": "1"},
  }
  15: map<TestEnum, switch_config.L4PortRange> mapOfEnumToStruct;
  16: map<string, switch_config.L4PortRange> mapOfStringToStruct;
  @thrift.DeprecatedUnvalidatedAnnotations{
    items = {"allow_skip_thrift_cow": "1"},
  }
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
  @thrift.DeprecatedUnvalidatedAnnotations{
    items = {"allow_skip_thrift_cow": "1"},
  }
  27: map<i32, bool> hybridMap;
  @thrift.DeprecatedUnvalidatedAnnotations{
    items = {"allow_skip_thrift_cow": "1"},
  }
  28: list<i32> hybridList;
  @thrift.DeprecatedUnvalidatedAnnotations{
    items = {"allow_skip_thrift_cow": "1"},
  }
  29: set<i32> hybridSet;
  @thrift.DeprecatedUnvalidatedAnnotations{
    items = {"allow_skip_thrift_cow": "1"},
  }
  30: TestUnion hybridUnion;
  @thrift.DeprecatedUnvalidatedAnnotations{
    items = {"allow_skip_thrift_cow": "1"},
  }
  31: ChildStruct hybridStruct;
  // hybridMapOfI32ToStruct is crafted to cover deeper accesses inside HybridNode, with
  // paths that terminate at primitive leaves, intermediate containers (list, map, struct)
  // in UTs for various visitors: PathVisitor, RecurseVisitor, DeltaVisitor
  @thrift.DeprecatedUnvalidatedAnnotations{
    items = {"allow_skip_thrift_cow": "1"},
  }
  32: map<i32, ChildStruct> hybridMapOfI32ToStruct;
  @thrift.DeprecatedUnvalidatedAnnotations{
    items = {"allow_skip_thrift_cow": "1"},
  }
  33: map<i32, map<i32, i32>> hybridMapOfMap;
  @thrift.DeprecatedUnvalidatedAnnotations{
    items = {"allow_skip_thrift_cow": "1"},
  }
  34: map<i32, set<string>> mapOfI32ToSetOfString;
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
@thrift.DeprecatedUnvalidatedAnnotations{items = {"random_annotation": "1"}}
struct TestStructForAnnotation1 {
  1: TestStruct childStruct;
}

@thrift.DeprecatedUnvalidatedAnnotations{
  items = {"allow_skip_thrift_cow": "0", "deprecated": "1"},
}
struct TestStruct2 {
  @thrift.DeprecatedUnvalidatedAnnotations{items = {"deprecated": "1"}}
  10: i32 deprecatedField;
}

@thrift.DeprecatedUnvalidatedAnnotations{items = {"allow_skip_thrift_cow": "1"}}
struct TestStruct3 {
  1: i32 inlineInt;
}

// structs declared for testing annotation on Struct
struct AnotherRootStruct {
  1: TestStruct2 childCowStruct;
  2: TestStruct3 childHybridStruct;
  3: map<i32, TestStruct3> mapOfHybridStruct;
  4: list<TestStruct3> listOfHybridStruct;
}
