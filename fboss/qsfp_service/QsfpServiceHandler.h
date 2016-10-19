#pragma once

#include <folly/futures/Future.h>

#include "common/fb303/cpp/FacebookBase2.h"

#include "fboss/qsfp_service/if/gen-cpp2/QsfpService.h"
#include "fboss/qsfp_service/TransceiverManager.h"

namespace facebook { namespace fboss {
class QsfpServiceHandler : public facebook::fboss::QsfpServiceSvIf,
                   public facebook::fb303::FacebookBase2 {
 public:
  explicit QsfpServiceHandler(std::unique_ptr<TransceiverManager> manager);
  ~QsfpServiceHandler(){}
  void init();
  facebook::fb303::cpp2::fb_status getStatus() override;
  TransceiverType type(int16_t idx) override;
  /*
   * Returns all qsfp information for the transceiver
   */
  void getTransceiverInfo(std::map<int32_t, TransceiverInfo>& info,
      std::unique_ptr<std::vector<int32_t>> ids) override;
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
  void customizeTransceiver(int16_t idx, cfg::PortSpeed speed) override;
 private:
  // Forbidden copy constructor and assignment operator
  QsfpServiceHandler(QsfpServiceHandler const &) = delete;
  QsfpServiceHandler& operator=(QsfpServiceHandler const &) = delete;

  std::unique_ptr<TransceiverManager> manager_;
};
}} // facebook::fboss
