namespace cpp2 facebook.fboss.utility
namespace go neteng.fboss.agent_hw_test_ctrl
namespace php fboss
namespace py neteng.fboss.agent_hw_test_ctrl
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.agent_hw_test_ctrl

include "fboss/agent/switch_state.thrift"
include "fboss/agent/switch_config.thrift"

service AgentHwTestCtrl {
  // acl utils begin
  i32 getDefaultAclTableNumAclEntries();

  i32 getAclTableNumAclEntries(1: string name);

  bool isDefaultAclTableEnabled();

  bool isAclTableEnabled(1: string name);

  bool isAclEntrySame(1: switch_state.AclEntryFields aclEntry);

  bool areAllAclEntriesEnabled();

  bool isStatProgrammedInDefaultAclTable(
    1: list<string> aclEntryNames,
    2: string counterName,
    3: list<switch_config.CounterType> types,
  );

  bool isStatProgrammedInAclTable(
    1: list<string> aclEntryNames,
    2: string counterName,
    3: list<switch_config.CounterType> types,
    4: string tableName,
  );
}
