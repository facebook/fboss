namespace cpp facebook.fboss.thrift
namespace cpp2 facebook.fboss.thrift
namespace go neteng.fboss
namespace php fboss
namespace py neteng.fboss
namespace py3 neteng.fboss
namespace py.asyncio neteng.asyncio.fboss

include "thrift/annotation/thrift.thrift"

@thrift.DeprecatedUnvalidatedAnnotations{items = {"cpp.virtual": "1"}}
exception FbossBaseError {
  @thrift.ExceptionMessage
  1: string message;
}
