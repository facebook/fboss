#pragma once

#include <boost/container/flat_map.hpp>

#include "fboss/lib/usb/WedgeI2CBus.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeI2CBusLock.h"
#include "fboss/qsfp_service/TransceiverManager.h"

namespace facebook { namespace fboss {
class WedgeManager : public TransceiverManager {
 public:
  WedgeManager();
  virtual ~WedgeManager() override {}
  void initTransceiverMap() override;
 private:
  // Forbidden copy constructor and assignment operator
  WedgeManager(WedgeManager const &) = delete;
  WedgeManager& operator=(WedgeManager const &) = delete;
 protected:
  virtual std::unique_ptr<BaseWedgeI2CBus> getI2CBus();
  virtual int getNumQsfpModules() { return 16; }

  std::unique_ptr<WedgeI2CBusLock> wedgeI2CBusLock_;
};
}} // facebook::fboss
