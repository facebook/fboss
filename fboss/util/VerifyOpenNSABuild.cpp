/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

/*
 * This is minimal code that can be used to verify whether an application can
 * be built and linked with opennsa code successfully.
 *
 * This is particularly useful while verifying opennsa EA drop shared by
 * Broadcom before they formally release opennsa on github.
 *
 * In the past, we have seen libopennsa.a link successfully on CentOS but fail
 * on Ubuntu 18.04. Thus, it would be a good idea to try below steps on CentOS
 * as well as Ubuntu.
 *
 * mkdir opennsa-verify
 * copy this file (VerifyOpenNSABuild.cpp) to opennsa-verify
 * copy and extract opennsa tarball to verify in opennsa-verify
 *
 * cd opennsa-verify
 * g++ VerifyOpenNSABuild.cpp -I $PWD/opennsa/include -I $PWD/opennsa/src/sdk/gpl-modules -L $PWD/lib/x86-64 -l opennsa -l pthread -lrt
 * ldd a.out # should show dependency on libopennsa.so
 * ./a.out
 *
 * rm -rf a.out $PWD/lib/x86-64/libopennsa.so
 * g++ VerifyOpenNSABuild.cpp -I $PWD/opennsa/include -I $PWD/opennsa/src/sdk/gpl-modules -L $PWD/lib/x86-64 -l opennsa -l pthread -lrt
 * ldd a.out # should not show dependency on libopennsa.so, statically linked with libopennsa.a
 * ./a.out
 *
 * If static linking fails (say recompile with -fPIC error), try passing -static to g++.
 * While that may succeed, it is still a bug in libopennsa.a that needs to be investigated.
 */
extern "C" {

#include <bcm/stat.h>
#include <bcm/error.h>
#include <include/ibde.h>

}

const char *_build_release = "unknown";
const char *_build_user = "unknown";
const char *_build_host = "unknown";
const char *_build_date = "no date";
const char *_build_datestamp = "no datestamp";
const char *_build_tree = "unknown";
const char *_build_arch = "unknown";
const char *_build_os = "unknown";

/*
 * bde_create() must be defined as a symbol when linking against BRCM libs.
 * It should never be invoked in our setup though. So return a error
 */
extern "C" int bde_create() {
  return BCM_E_UNAVAIL;
}
/*
 *  We don't set any default values.
 */
extern "C" void sal_config_init_defaults() {}

/*
 * Define the bde global variable.
 * This is declared in <ibde.h>, but needs to be defined by our code.
 */
ibde_t* bde;

int main() {
  bcm_stat_sync(0);
  return 0;
}'
