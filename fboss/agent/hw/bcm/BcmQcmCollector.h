#pragma once

#include "fboss/agent/hw/bcm/BcmQcmManager.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/types.h"

extern "C" {
#include <bcm/flowtracker.h>
#include <bcm/l2.h>
}

namespace facebook::fboss {

class BcmSwitch;
class QcmCfg;

class BcmQcmCollector {
 public:
  explicit BcmQcmCollector(BcmSwitch* hw, BcmQcmManager* qcmMgr)
      : hw_(hw), qcmMgr_(qcmMgr) {}
  ~BcmQcmCollector() {}

  // Initialize collector specific routines
  void init(const std::shared_ptr<SwitchState>& swState);
  void stop();

  static folly::MacAddress getCollectorDstMac();
  static int getQcmCollectorExportProfileId();
  static int getQcmCollectorId();

 private:
  BcmSwitch* hw_;
  BcmQcmManager* qcmMgr_;
  bcm_flowtracker_export_template_t flowtrackerTemplateId_{1};
  int collector_vlan_;
  std::shared_ptr<QcmCfg> qcmCfg_; // remove once we start using thrift directly

  void populateCollectorIPInfo(bcm_collector_info_t& collectorInfo);
  void collectorTemplateAttach();
  void setupL3Collector();
  bool getCollectorVlan(const std::shared_ptr<SwitchState>& swState);
  void collectorCreate();
  void exportProfileCreate();
  void flowTrackerTemplateCreate();

  // destory routines
  void collectorTemplateDetach();
  void destroyCollector();
  void destroyExportProfile();
  void destroyFlowTrackerTemplate();
};
} // namespace facebook::fboss
