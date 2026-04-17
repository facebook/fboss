namespace cpp2 facebook.fboss.platform.reboot_cause_service
namespace go neteng.fboss.platform.reboot_cause_service
namespace php NetengFbossPlatformRebootCauseService
namespace py neteng.fboss.platform.reboot_cause_service
namespace py3 neteng.fboss.platform.reboot_cause_service
namespace py.asyncio neteng.fboss.platform.asyncio.reboot_cause_service

include "fboss/agent/if/fboss.thrift"
include "fboss/platform/reboot_cause_service/if/reboot_cause_config.thrift"
include "thrift/annotation/thrift.thrift"

@thrift.AllowLegacyMissingUris
package;

// RebootCauseData models a reboot cause
// description: a human-readable description of the cause
// date: a human-readable date string of when cause occurred
// externalProvider: the name of an external provider that is responsible for this cause
struct RebootCauseData {
  1: string description;
  2: string date;
  3: optional string externalProvider;
}

// RebootCauseProvider models a reboot cause provider and its read causes
// name: the name of the provider
// config: the configuration of the provider
// causes: any reboot causes read from the provider
struct RebootCauseProvider {
  1: string name;
  2: reboot_cause_config.RebootCauseProviderConfig config;
  3: list<RebootCauseData> causes;
}

// RebootCauseResult models the result of a reboot cause investigation
// date: the human-readable date string of when investigation ran
// determinedCause: cause determined to be responsible for reboot
// determinedCauseProvider: the name of the provider of the determined reboot cause
// providers: all providers that reported causes for this investigation
struct RebootCauseResult {
  1: string date;
  2: RebootCauseData determinedCause;
  3: string determinedCauseProvider;
  4: list<RebootCauseProvider> providers;
}

// RebootCauseService is the service API
// getLastRebootCause: returns the result of the last reboot cause investigation
service RebootCauseService {
  RebootCauseResult getLastRebootCause()
    throws (1: fboss.FbossBaseError error);
}
