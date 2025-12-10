package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

struct ShowAgentSslModel {
  1: string AgentSslStatus;
}

struct AgentFirmwareEntry {
  1: string version;
  2: string opStatus;
  3: string funcStatus;
}

struct ShowAgentFirmwareModel {
  1: list<AgentFirmwareEntry> firmwareEntries;
}
