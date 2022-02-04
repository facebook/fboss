// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/SflowShimUtils.h"

namespace facebook::fboss::utility {

SflowShimHeaderInfo parseSflowShim(folly::io::Cursor& cursor) {
  SflowShimHeaderInfo shim{};

  shim.asic = SflowShimAsic::SFLOW_SHIM_ASIC_UNKNOWN;

  uint8_t srcPort = cursor.readBE<uint8_t>();
  uint8_t srcModId = cursor.readBE<uint8_t>();
  uint8_t dstPort = cursor.readBE<uint8_t>();
  uint8_t dstModId = cursor.readBE<uint8_t>();

  /*
   * In BRCM TH4 ASIC we have a 32 bit version field at the start and
   * rest of the fields is same as that of TH3.
   *
   * And we also confirmed from BRCM that unless the sampled packet is
   * ingressing and egressing from the CPU port,
   * All four fields (srcPort, srcModId, dstPort, dstModid) won't be 0.
   *
   * And we also verified that version field in TH4 sample packet will be 0.
   *
   * So, if the below calculated potentialVersionField is 0, we can safely
   * assume that the packet is from TH4 ASIC
   */
  uint32_t potentialVersionField =
      srcPort << 24 | srcModId << 16 | dstPort << 8 | dstModId;
  if (potentialVersionField == 0) {
    // This is a coming from TH4 ASIC.
    shim.asic = SflowShimAsic::SFLOW_SHIM_ASIC_TH4;
    srcPort = cursor.readBE<uint8_t>();
    // Src Module ID not needed
    cursor.readBE<uint8_t>();
    dstPort = cursor.readBE<uint8_t>();
    // Src Module ID not needed
    cursor.readBE<uint8_t>();
  } else {
    shim.asic = SflowShimAsic::SFLOW_SHIM_ASIC_TH3;
  }
  // don't need sflow flags + reserved bits
  cursor.readBE<uint32_t>();

  // For shim header, we store source/dest ports in the interface fields.
  // The ports will later be translated into interface names.
  shim.srcPort = srcPort;
  shim.dstPort = dstPort;

  return shim;
}

} // namespace facebook::fboss::utility
