// Copyright 2004-present Facebook. All Rights Reserved.
#pragma once

#include <string>

extern "C" {
#include <bcm/rx.h>
}

namespace facebook::fboss {

class RxUtils {
 public:
  /*
   * Generate a human-readable string describing a bcm_rx_reasons_t.
   */
  static std::string describeReasons(bcm_rx_reasons_t reasons);

  /*
   * Generate a bcm_rx_reasons_t with the specified reasons set.
   */
  template <class... ReasonEnums>
  static bcm_rx_reasons_t genReasons(ReasonEnums... r) {
    bcm_rx_reasons_t reasons;
    bcm_rx_reasons_t_init(&reasons);
    addReasons(&reasons, r...);
    return reasons;
  }

 private:
  // Forbidden copy constructor and assignment operator
  RxUtils(RxUtils const&) = delete;
  RxUtils& operator=(RxUtils const&) = delete;

  static void addReasons(bcm_rx_reasons_t* /*reasons*/) {
    // no-op,
    // This terminates recursive template expansion,
    // and it is also used when generating an empty reasons set.
    // to support generating an empty reasons set.
  }
  template <class... ReasonEnums>
  static void addReasons(
      bcm_rx_reasons_t* reasons,
      bcm_rx_reason_e first,
      ReasonEnums... rest) {
    BCM_RX_REASON_SET((*reasons), first);
    addReasons(reasons, rest...);
  }
};

} // namespace facebook::fboss
