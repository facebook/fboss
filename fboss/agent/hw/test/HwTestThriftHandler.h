#include "fboss/agent/if/gen-cpp2/AgentHwTestCtrl.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/HwSwitch.h"

#include "fboss/agent/gen-cpp2/switch_state_types.h"

#include <memory>

namespace facebook::fboss {
class HwSwitch;
}
namespace facebook::fboss::utility {
class HwTestThriftHandler : public AgentHwTestCtrlSvIf {
 public:
  explicit HwTestThriftHandler(HwSwitch* hwSwitch) : hwSwitch_(hwSwitch) {}

  int32_t getDefaultAclTableNumAclEntries() override;

  int32_t getAclTableNumAclEntries(
      std::unique_ptr<std::string> /* name */) override {
    throw FbossError("Not implemented");
  }

  bool isDefaultAclTableEnabled() override {
    throw FbossError("Not implemented");
  }

  bool isAclTableEnabled(std::unique_ptr<std::string> /* name */) override {
    throw FbossError("Not implemented");
  }

  bool isAclEntrySame(
      std::unique_ptr<state::AclEntryFields> /* aclEntry */) override {
    throw FbossError("Not implemented");
  }

  bool areAllAclEntriesEnabled() override {
    throw FbossError("Not implemented");
  }

  bool isStatProgrammedInDefaultAclTable(
      std::unique_ptr<std::vector<::std::string>> /*aclEntryNames*/,
      std::unique_ptr<std::string> /*counterName*/,
      std::unique_ptr<std::vector<cfg::CounterType>> /*types*/) override {
    throw FbossError("Not implemented");
  }

  bool isStatProgrammedInAclTable(
      std::unique_ptr<std::vector<::std::string>> /*aclEntryNames*/,
      std::unique_ptr<std::string> /*counterName*/,
      std::unique_ptr<std::vector<cfg::CounterType>> /*types*/,
      std::unique_ptr<std::string> /*tableName*/) override {
    throw FbossError("Not implemented");
  }

 private:
  HwSwitch* hwSwitch_;
};

std::shared_ptr<HwTestThriftHandler> createHwTestThriftHandler(HwSwitch* hw);
} // namespace facebook::fboss::utility
