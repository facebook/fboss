#pragma once

#include <folly/futures/Future.h>

#include "common/fb303/cpp/FacebookBase2.h"

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/QsfpService.h"
#include "fboss/qsfp_service/TransceiverManager.h"

namespace facebook { namespace fboss {

class QsfpServiceHandler : public facebook::fboss::QsfpServiceSvIf,
                           public facebook::fb303::FacebookBase2 {
 public:
  explicit QsfpServiceHandler(std::unique_ptr<TransceiverManager> manager);
  ~QsfpServiceHandler() override = default;

  void init();
  facebook::fb303::cpp2::fb_status getStatus() override;
  TransceiverType getType(int32_t idx) override;
  /*
   * Returns all qsfp information for the transceiver
   */
  void getTransceiverInfo(std::map<int32_t, TransceiverInfo>& info,
      std::unique_ptr<std::vector<int32_t>> ids) override;

  /*
   * Returns raw DOM page data for each passed in transceiver.
   */
  void getTransceiverRawDOMData(
    std::map<int32_t, RawDOMData>& info,
    std::unique_ptr<std::vector<int32_t>> ids) override;

  /*
   * Returns a union of the two raw DOM data format for each passed in transceiver.
   */
  void getTransceiverDOMDataUnion(
    std::map<int32_t, DOMDataUnion>& info,
    std::unique_ptr<std::vector<int32_t>> ids) override;


  /*
   * Store port status information and return relevant transceiver map.
   */
  void syncPorts(
    std::map<int32_t, TransceiverInfo>& info,
    std::unique_ptr<std::map<int32_t, PortStatus>> ports) override;

  /*
   * Customise the transceiver based on the speed at which it has
   * been configured to operate at
   */
  void customizeTransceiver(int32_t idx, cfg::PortSpeed speed) override;

  /*
   * Return a pointer to the transceiver manager.
   */
  TransceiverManager* getTransceiverManager() const {
    return manager_.get();
  }

 private:
  // Forbidden copy constructor and assignment operator
  QsfpServiceHandler(QsfpServiceHandler const &) = delete;
  QsfpServiceHandler& operator=(QsfpServiceHandler const &) = delete;

  std::unique_ptr<TransceiverManager> manager_{nullptr};
};
}} // facebook::fboss
