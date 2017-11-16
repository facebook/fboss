namespace facebook {
namespace fboss {
class NetlinkManagerException : public std::runtime_error {
 public:
  explicit NetlinkManagerException(const std::string& ex)
      : std::runtime_error(ex) {}
};
} // namespace fboss
} // namespace facebook
