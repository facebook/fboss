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

  int32_t getAclTableNumAclEntries(std::unique_ptr<std::string> name) override;

  bool isDefaultAclTableEnabled() override;

  bool isAclTableEnabled(std::unique_ptr<std::string> name) override;

  bool isAclEntrySame(
      std::unique_ptr<state::AclEntryFields> aclEntry,
      std::unique_ptr<std::string> aclTableName) override;

  bool areAllAclEntriesEnabled() override {
    throw FbossError("Not implemented");
  }

  bool isStatProgrammedInDefaultAclTable(
      std::unique_ptr<std::vector<::std::string>> aclEntryNames,
      std::unique_ptr<std::string> counterName,
      std::unique_ptr<std::vector<cfg::CounterType>> types) override;

  bool isStatProgrammedInAclTable(
      std::unique_ptr<std::vector<::std::string>> aclEntryNames,
      std::unique_ptr<std::string> counterName,
      std::unique_ptr<std::vector<cfg::CounterType>> types,
      std::unique_ptr<std::string> tableName) override;

 private:
  HwSwitch* hwSwitch_;
};

std::shared_ptr<HwTestThriftHandler> createHwTestThriftHandler(HwSwitch* hw);
} // namespace facebook::fboss::utility
