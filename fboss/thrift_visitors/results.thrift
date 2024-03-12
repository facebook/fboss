namespace py3 neteng.fboss
namespace py neteng.fboss.results
namespace py.asyncio neteng.fboss.asyncio.results
namespace cpp2 facebook.fboss.fsdb
namespace go facebook.fboss.results

enum NameToPathResult {
  OK = 0,
  INVALID_PATH = 1,
  INVALID_STRUCT_MEMBER = 2,
  INVALID_VARIANT_MEMBER = 3,
  INVALID_ARRAY_INDEX = 4,
  INVALID_MAP_KEY = 5,
  VISITOR_EXCEPTION = 6,
  UNSUPPORTED_WILDCARD_PATH = 7,
}
