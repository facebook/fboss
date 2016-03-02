# Build Instructions

FBOSS is being tested on Ubuntu 14.04.

The provided `getdeps.sh` script will fetch the dependencies required. If they are
available as Ubuntu packages, then they will be installed through apt-get. If they
are not, then they will be downloaded to external/ and built from source.

If you want to build them manually, the dependencies are as follows:

* [folly](https://github.com/facebook/folly), built from source and installed
* folly's own prerequisites: double-conversion, gflags, and glog
* [fbthrift](https://github.com/facebook/fbthrift), built from source and
  installed
* iproute2-3.19.0: download from
  [here](https://www.kernel.org/pub/linux/utils/net/iproute2/iproute2-3.19.0.tar.xz)
* Broadcom's [OpenNSL](https://github.com/Broadcom-Switch/OpenNSL)
* [OCP-SAI](https://github.com/opencomputeproject/SAI.git), download only.

Once the prerequisites are available, take the following steps. 

Clone the repository:

```
git clone https://github.com/facebook/fboss.git
```
 
If you installed dependencies manually, CMAKE_INCLUDE_PATH and
CMAKE_LIBRARY_PATH must be modified to include the paths to the dependencies.
If you use `getdeps.sh`, then this should work out of the box. 

If you build with SAI there are two additional steps: (1) copy and rename the 
SAI adapter library as external/SAI/lib/libSaiAdapter.so (2) copy and rename
./fboss/github/CMakeLists_SAI.txt as ./CMakeLists_SAI.txt.

Build as follows:

```
mkdir fboss/build
cd fboss/build
cmake ..
make
```
The produced executables are `sim_agent` and `wedge_agent` (or `sai_agent`) in the build directory.
