// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/qsfp_service/util/MacsecUtils.h"

namespace facebook::fboss::utility {

mka::MKASci makeSci(const std::string& mac, PortID portId) {
  mka::MKASci sci;
  sci.macAddress_ref() = mac;
  sci.port_ref() = portId;
  return sci;
}

mka::MKASak makeSak(
    const mka::MKASci& sci,
    std::string portName,
    const std::string& keyHex,
    const std::string& keyIdHex,
    int assocNum) {
  mka::MKASak sak;
  sak.sci_ref() = sci;
  sak.l2Port_ref() = portName;
  sak.keyHex_ref() = keyHex;
  sak.keyIdHex_ref() = keyIdHex;
  sak.assocNum_ref() = assocNum;
  return sak;
}

std::string getAclName(
    facebook::fboss::PortID port,
    sai_macsec_direction_t direction) {
  return folly::to<std::string>(
      "macsec-",
      direction == SAI_MACSEC_DIRECTION_INGRESS ? "ingress" : "egress",
      "-port",
      port);
}

MacsecSecureChannelId packSecureChannelId(const mka::MKASci& sci) {
  folly::MacAddress mac = folly::MacAddress(*sci.macAddress_ref());
  return MacsecSecureChannelId(mac.u64NBO() | *sci.port_ref());
}

} // namespace facebook::fboss::utility
