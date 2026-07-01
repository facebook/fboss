namespace cpp2 facebook.fboss.platform.reboot_cause_config
namespace go neteng.fboss.platform.reboot_cause_config
namespace php NetengFbossPlatformRebootCauseConfig
namespace py neteng.fboss.platform.reboot_cause_config
namespace py3 neteng.fboss.platform.reboot_cause_config
namespace py.asyncio neteng.fboss.platform.asyncio.reboot_cause_config

include "thrift/annotation/thrift.thrift"

@thrift.AllowLegacyMissingUris
package;

enum RebootCauseProviderType {
  KERNEL_DRIVER_JSON = 0,
}

// RebootCauseProviderConfig defines a hardware provider of a reboot cause
// type: the format of the reboot causes returned by sysfsReadPath
// priority: the importance of the provider in the system (lower is more important)
// sysfsReadPath: the sysfs path from which to read reboot causes
// sysfsClearPath: the sysfs path to write to clear reboot causes
struct RebootCauseProviderConfig {
  1: RebootCauseProviderType type;
  2: i16 priority;
  3: string sysfsReadPath;
  4: string sysfsClearPath;
}

// RebootCauseConfig defines the reboot_cause_service configuration for the platform
// rebootCauseProviderConfigs: platform-specific providers of reboot causes
struct RebootCauseConfig {
  1: map<string, RebootCauseProviderConfig> rebootCauseProviderConfigs;
}
