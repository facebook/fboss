// Copyright 2004-present Facebook. All Rights Reserved.
//
#include "thrift/lib/cpp2/async/AsyncProcessor.h"
#include "thrift/lib/cpp2/server/ThriftServer.h"

DECLARE_bool(help);
DECLARE_string(helpmatch);

namespace facebook::fboss::platform::helpers {

void init(int* argc, char*** argv);

// `helpmatch` controls the help message that is printed when running with
// --help. Refer to FLAGS_helpmatch in gflags.cc for more details.
void initCli(int* argc, char*** argv, const std::string& helpmatch);

void runThriftService(
    std::shared_ptr<apache::thrift::ThriftServer> server,
    std::shared_ptr<apache::thrift::ServerInterface> handler,
    const std::string& serviceName,
    uint32_t port);

} // namespace facebook::fboss::platform::helpers
