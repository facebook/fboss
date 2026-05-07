namespace py3 neteng
namespace cpp2 facebook.bgp.thrift
namespace py neteng.bgp
namespace py.asyncio neteng.asyncio.bgp

include "thrift/annotation/thrift.thrift"

@thrift.AllowLegacyMissingUris
package;

@thrift.DeprecatedUnvalidatedAnnotations{items = {"cpp.virtual": "1"}}
exception BgpBaseError {
  @thrift.ExceptionMessage
  1: string message;
}
