include "thrift/annotation/thrift.thrift"
include "thrift/annotation/hack.thrift"

@hack.NamePrefix{prefix = "fboss_bcm_config_"}
@hack.LegacyOmitPrefixInNameString
@thrift.AllowLegacyMissingUris
package;

namespace py neteng.fboss.bcm_config
namespace py3 neteng.fboss.bcm_config
namespace py.asyncio neteng.fboss.asyncio.bcm_config
namespace cpp2 facebook.fboss.bcm
namespace go neteng.fboss.bcm_config

struct BcmConfig {
  1: map<string, string> config;
  2: optional string yamlConfig;
}
