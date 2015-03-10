# Build Instructions

FBOSS is being tested on Ubuntu 14.04. To install and build the FBOSS agent, you need the following prerequisites:

* [folly](https://github.com/facebook/folly), built from source and installed
* folly's own prerequisites: double-conversion, gflags, and glog
* [fbthrift](https://github.com/facebook/fbthrift), built from source and installed
* iproute2-3.19.0: download from [here](https://www.kernel.org/pub/linux/utils/net/iproute2/iproute2-3.19.0.tar.xz), extract, and install per instructions
* Broadcom's [OpenNSL](https://github.com/Broadcom-Switch/OpenNSL)

Once the prerequisites are available, take the following steps.

Clone the repository:

```
git clone https://github.com/facebook/fboss.git
```
 
Build as follows:

```
cd fboss
make OPENNSL_INCLUDE=/path/to/opennsl/include OPENNSL_LIB=/path/to/libopennsl.so.1 \
  IPROUTE2_LIB=/path/to/libnetlink.a
```

Please note that `OPENNSL_INCLUDE` is a directory containing the opennsl include files, whereas `OPENNSL_LIB` and `IPROUTE2_LIB` are actual file names (i.e. not only the directories where they may be found). Parallelization using `-j` works, but maintenance builds (e.g. after changing a header file) are not fully supported&mdash;you may want to run `make clean` first if you change source files between builds.

The produced executables are `sim_agent` and `wedge_agent` in the current directory.
