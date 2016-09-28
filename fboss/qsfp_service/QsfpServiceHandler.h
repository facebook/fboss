#pragma once

#include "fboss/qsfp_service/if/gen-cpp2/QsfpService.h"

#include "common/fb303/cpp/FacebookBase2.h"

namespace facebook { namespace fboss {
class QsfpServiceHandler : public facebook::fboss::QsfpServiceSvIf,
                   public facebook::fb303::FacebookBase2 {
 public:
  QsfpServiceHandler();
  ~QsfpServiceHandler(){}
  facebook::fb303::cpp2::fb_status getStatus() override;
  TransceiverType type(int16_t idx) override;
  /*
   * Returns all qsfp information for the transceiver
   */
  void getTransceiverInfo(TransceiverInfo& info, int16_t idx) override;
  /*
   * Whether or not the qsfp is present for that transceiver
   */
  bool isTransceiverPresent(int16_t idx) override;
  /*
   * Force a refresh of the cache for a transceiver
   */
  void updateTransceiverInfoFields(int16_t idx) override;
  /*
   * Customise the transceiver based on the speed at which it has
   * been configured to operate at
   */
  void customizeTransceiver(int16_t idx, cfg::cpp2::PortSpeed speed) override;
 private:
  // Forbidden copy constructor and assignment operator
  QsfpServiceHandler(QsfpServiceHandler const &) = delete;
  QsfpServiceHandler& operator=(QsfpServiceHandler const &) = delete;
};
}} // facebook::fboss
