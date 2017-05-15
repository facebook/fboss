# Build Instructions

FBOSS is being tested on Ubuntu 16.04.

Required packages
* cmake3.0.2 or higher
* gcc/g++ 4.9+ (with c++14 support)

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

Once the prerequisites are available, take the following steps.

Clone the repository:

```
git clone https://github.com/facebook/fboss.git
```

If you installed dependencies manually, CMAKE_INCLUDE_PATH and
CMAKE_LIBRARY_PATH must be modified to include the paths to the dependencies.
If you use `getdeps.sh`, then this should work out of the box.

Build as follows:

```
mkdir fboss/build
cd fboss/build
cmake ..
make
```

FYI: the build status of open source fboss is tracked at:
   https://travis-ci.org/facebook/fboss

The produced executables are `sim_agent` and `wedge_agent` in the build directory.
