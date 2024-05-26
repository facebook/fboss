namespace py3 neteng.fboss.thrift_cow
namespace py neteng.fboss.thrift_cow.results
namespace py.asyncio neteng.fboss.thrift_cow.asyncio.results
namespace cpp2 facebook.fboss.thrift_cow
namespace go facebook.fboss.thrift_cow.results

enum PatchApplyResult {
  OK = 0,
  INVALID_STRUCT_MEMBER = 1,
  INVALID_VARIANT_MEMBER = 2,
  INVALID_PATCH_TYPE = 3,
  NON_EXISTENT_NODE = 4,
  KEY_PARSE_ERROR = 5,
  PATCHING_IMMUTABLE_NODE = 6,
}
