// Copyright 2014-present Facebook. All Rights Reserved.

#include <fboss/platform/data_corral_service/FruModule.h>

namespace facebook::fboss::platform::data_corral_service {

class DarwinFanModule : public FruModule {
 public:
  void refresh() override;
};

} // namespace facebook::fboss::platform::data_corral_service
