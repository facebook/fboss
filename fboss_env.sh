export CPLUS_INCLUDE_PATH=/usr/include/python3.6m/
export PATH=/usr/bin:$PATH
export SAI_TAJO_IMPL=1
export YACC=bison
export BENCHMARK_INSTALL=0
export FBOSS_CENTOS9=1
export SAI_VERSION="${SAI_VERSION:-1.16.1}"
export SAI_ONLY=1
# Set TAJO_SDK_VERSION based on SAI_VERSION (skip if already exported)
if [ -n "$TAJO_SDK_VERSION" ]; then
  echo "Using pre-set TAJO_SDK_VERSION: ${TAJO_SDK_VERSION}"
elif [ "$SAI_VERSION" = "1.14.0" ]; then
  export TAJO_SDK_VERSION="24_8_3001"
elif [ "$SAI_VERSION" = "1.16.1" ]; then
  export TAJO_SDK_VERSION="25_5_4210"
elif [ "$SAI_VERSION" = "1.17.0" ]; then
  export TAJO_SDK_VERSION="25_11_4210"
else
  export TAJO_SDK_VERSION="26_2_4210"
fi
echo "TAJO_SDK_VERSION: ${TAJO_SDK_VERSION}"
