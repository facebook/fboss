include "thrift/annotation/thrift.thrift"
include "fboss/qsfp_service/if/transceiver.thrift"
include "fboss/lib/phy/phy.thrift"

@thrift.AllowLegacyMissingUris
package;

namespace cpp2 facebook.fboss
namespace go neteng.fboss.link_qsfp_test_port_info
namespace php fboss_link_qsfp_test_port_info
namespace py neteng.fboss.link_qsfp_test_port_info
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.link_qsfp_test_port_info

struct LinkQsfpTestPortInfo {
  1: string testName;
  2: i32 portId;
  3: string portName;
  4: i32 transceiverId;
  5: string vendorName;
  6: string vendorPartNumber;
  7: string vendorSerialNumber;
  8: string fwVersion;
  9: string dspFwVersion;
  // Module level media interface (TcvrState.moduleMediaInterface)
  10: transceiver.MediaInterfaceCode moduleMediaInterface;
  // Per-port media interface from TransceiverSettings.mediaInterface[lane]
  11: transceiver.MediaInterfaceCode portMediaInterface;
  12: optional phy.FecMode fecMode;
  13: map<string, string> extraMetadata;
  // Production feature(s) the test verifies (enum names from
  // QsfpProductionFeature). Populated by qsfp hw tests via
  // HwTest::addVerifiedProductionFeatures(); empty for tests that do not declare
  // any.
  14: list<string> verifiedProductionFeatures;
}
