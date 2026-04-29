package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

// A single reboot cause investigation result for display.
struct ShowRebootCauseModel {
  1: string investigationDate;   // when the service ran the investigation
  2: string causeDescription;    // human-readable cause (e.g. "PSU AC Loss")
  3: string causeDate;           // when the hardware event occurred
  4: string causeProvider;       // which provider reported the cause
}

// History: a list of results, most recent first.
struct ShowRebootCauseHistoryModel {
  1: list<ShowRebootCauseModel> entries;
}
