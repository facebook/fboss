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
* OCP SAI [SAI-0.9.3](https://github.com/opencomputeproject/SAI/tree/v0.9.3.0)

Once the prerequisites are available, take the following steps. 

Clone the repository:

```
git clone https://github.com/facebook/fboss.git
```
 
If you installed dependencies manually, CMAKE_INCLUDE_PATH and
CMAKE_LIBRARY_PATH must be modified to include the paths to the dependencies.
If you use `getdeps.sh`, then this should work out of the box. If you build with 
SAI copy your platform specific libSaiAdapter.so to external/SAI/lib directory 
or modify path to the library file.

Build as follows:

```
mkdir fboss/build
cd fboss/build
cmake ..
make
```

Build as follows (for SAI Agent)
```
mkdir fboss/build
cd fboss/build
cmake .. -DWITH_SAI:BOOL=ON
make
```

The produced executables are `sim_agent` and `wedge_agent` or `sai_agent` (when
building with SAI) in the build directory.
