include "thrift/annotation/thrift.thrift"
include "thrift/annotation/cpp.thrift"

namespace py3 neteng.fboss.thrift_cow
namespace py neteng.fboss.thrift_cow.patch
namespace py.asyncio neteng.fboss.asyncio.thrift_cow.patch
namespace cpp2 facebook.fboss.thrift_cow
namespace go facebook.fboss.thrift_cow

@cpp.Type{name = "folly::IOBuf"}
typedef binary ByteBuffer
typedef byte Empty

struct StructPatch {
  1: map<i16, PatchNode> children;
  2: optional ByteBuffer compressedChildren;
}

struct MapPatch {
  1: map<string, PatchNode> children;
  2: optional ByteBuffer compressedChildren;
}

struct ListPatch {
  1: map<i32, PatchNode> children;
  2: optional ByteBuffer compressedChildren;
}

// keys for set children are the actual value. PatchNode will be either a val or del
// TODO: split this to an added + removed sets
struct SetPatch {
  1: map<string, PatchNode> children;
  2: optional ByteBuffer compressedChildren;
}

struct VariantPatch {
  1: i16 id;
  @thrift.Box // avoid circular dep
  2: optional PatchNode child;
}

union PatchNode {
  1: Empty del;
  2: ByteBuffer val;
  3: StructPatch struct_node;
  4: MapPatch map_node;
  5: ListPatch list_node;
  6: SetPatch set_node;
  7: VariantPatch variant_node;
}
