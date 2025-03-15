#include <string>
#include <vector>

namespace facebook::fboss::platform::fw_util {

void checkCmdStatus(const std::vector<std::string>& cmd, int exitStatus);
std::string getUpgradeToolBinaryPath(const std::string& toolName);
void verifySha1sum(const std::string& fpd, const std::string& configSha1sum);
std::string toLower(std::string);
std::string getGpioChip(const std::string&);

} // namespace facebook::fboss::platform::fw_util
