namespace cpp2 facebook.fboss.cli

struct ShowAgentSslModel {
  1: string AgentSslStatus;
}

struct ShowAgentFirmwareModel {
  1: string version;
  2: string opStatus;
  3: string funcStatus;
}
