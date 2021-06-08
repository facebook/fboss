# CMake to build libraries and binaries in fboss/cli/fboss2

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_fbthrift_cpp_library(
  show_transceiver_model
  fboss/cli/fboss2/commands/show/transceiver/model.thrift
)

add_executable(fboss2
  fboss/cli/fboss2/CmdClientUtils.cpp
  fboss/cli/fboss2/oss/CmdClientUtils.cpp
  fboss/cli/fboss2/commands/clear/CmdClearArp.h
  fboss/cli/fboss2/commands/clear/CmdClearNdp.h
  fboss/cli/fboss2/CmdGlobalOptions.cpp
  fboss/cli/fboss2/CmdHandler.cpp
  fboss/cli/fboss2/CmdList.cpp
  fboss/cli/fboss2/commands/show/CmdShowAcl.h
  fboss/cli/fboss2/commands/show/CmdShowArp.h
  fboss/cli/fboss2/commands/show/CmdShowLldp.h
  fboss/cli/fboss2/commands/show/CmdShowNdp.h
  fboss/cli/fboss2/commands/show/CmdShowNdp.h
  fboss/cli/fboss2/commands/show/CmdShowPort.h
  fboss/cli/fboss2/commands/show/CmdShowPortQueue.h
  fboss/cli/fboss2/commands/show/CmdShowInterface.h
  fboss/cli/fboss2/commands/show/transceiver/CmdShowTransceiver.h
  fboss/cli/fboss2/CmdSubcommands.cpp
  fboss/cli/fboss2/CmdUtils.cpp
  fboss/cli/fboss2/Main.cpp
  fboss/cli/fboss2/oss/CmdGlobalOptions.cpp
  fboss/cli/fboss2/oss/CmdList.cpp
  fboss/cli/fboss2/oss/CmdUtils.cpp
  fboss/cli/fboss2/options/SSLPolicy.h
)

target_link_libraries(fboss2
  CLI11
  ctrl_cpp2
  qsfp_cpp2
  Folly::folly
  show_transceiver_model
)

add_library(CLI11
  fboss/cli/fboss2/CLI11/App.hpp
  fboss/cli/fboss2/CLI11/CLI.hpp
  fboss/cli/fboss2/CLI11/Config.hpp
  fboss/cli/fboss2/CLI11/ConfigFwd.hpp
  fboss/cli/fboss2/CLI11/Error.hpp
  fboss/cli/fboss2/CLI11/Formatter.hpp
  fboss/cli/fboss2/CLI11/FormatterFwd.hpp
  fboss/cli/fboss2/CLI11/Macros.hpp
  fboss/cli/fboss2/CLI11/Option.hpp
  fboss/cli/fboss2/CLI11/Split.hpp
  fboss/cli/fboss2/CLI11/StringTools.hpp
  fboss/cli/fboss2/CLI11/Timer.hpp
  fboss/cli/fboss2/CLI11/TypeTools.hpp
  fboss/cli/fboss2/CLI11/Validators.hpp
  fboss/cli/fboss2/CLI11/Version.hpp
)

set_target_properties(CLI11 PROPERTIES LINKER_LANGUAGE CXX)

install(TARGETS fboss2)
