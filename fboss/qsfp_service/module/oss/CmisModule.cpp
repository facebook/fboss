#include "fboss/qsfp_service/module/cmis/CmisModule.h"

// TODO: delete this file once T230016502 is resolved.
namespace facebook {
namespace fboss {
bool CmisModule::isLpoModule() const {
  return false;
}

} // namespace fboss
} // namespace facebook
