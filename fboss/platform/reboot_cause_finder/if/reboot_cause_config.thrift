namespace cpp2 facebook.fboss.platform.reboot_cause_config
namespace go neteng.fboss.platform.reboot_cause_config
namespace php NetengFbossPlatformRebootCauseConfig
namespace py neteng.fboss.platform.reboot_cause_config
namespace py3 neteng.fboss.platform.reboot_cause_config
namespace py.asyncio neteng.fboss.platform.asyncio.reboot_cause_config

package "facebook.com/fboss/platform/reboot_cause_config"

// `RebootCauseProviderConfig` defines a hardware provider of a reboot cause.
//
// `name`: Unique name of the provider (e.g. MERU800BIA_SMB_CPLD).
//
// `priority`: Importance of the provider (lower value = higher priority).
//
// `sysfsReadPath`: Sysfs path to read reboot causes from.
//
// `sysfsClearPath`: Sysfs path to write to clear reboot causes.
struct RebootCauseProviderConfig {
  1: string name;
  2: i16 priority;
  3: string sysfsReadPath;
  4: string sysfsClearPath;
}

// `RebootCauseConfig` is the per-platform reboot_cause_finder configuration.
//
// `rebootCauseProviderConfigs`: Providers of reboot causes for the platform.
struct RebootCauseConfig {
  1: list<RebootCauseProviderConfig> rebootCauseProviderConfigs;
}

// `RebootCause` models one decoded cause read from one provider.
//
// `providerName`: Provider that reported this cause.
//
// `description`: Human-readable decoded cause from the provider.
//
// `occurredAtMs`: When the cause occurred, epoch milliseconds (canonical).
//
// `occurredAtPacific`: Same instant in Pacific time for humans, e.g.
// "2026-07-02 23:16:55 PDT". Display only; `occurredAtMs` is authoritative.
//
// `rawValue`: Raw undecoded register value, if the provider exposes it.
struct RebootCause {
  1: string providerName;
  2: string description;
  3: i64 occurredAtMs;
  4: string occurredAtPacific;
  5: optional string rawValue;
}

// `RebootCauseRecord` is the per-boot record persisted under
// /var/facebook/fboss/reboot_history/.
//
// `detectedAtMs`: When the finder ran, epoch milliseconds (canonical).
//
// `detectedAtPacific`: Detection time in Pacific for humans, e.g.
// "2026-07-02 23:22:12 PDT".
//
// `determinedCause`: The cause determined to be responsible for the reboot.
//
// `allCauses`: Every cause reported by every provider, for forensics.
struct RebootCauseRecord {
  1: i64 detectedAtMs;
  2: string detectedAtPacific;
  3: RebootCause determinedCause;
  4: list<RebootCause> allCauses;
}
