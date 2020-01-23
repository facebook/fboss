# FBOSS SAI

## Introduction
fboss/agent/hw/sai contains an implementation of the FBOSS HwSwitch interface
which uses the Open Compute Project's (OCP) Switch Abstraction Interface (SAI)
for programming the networking hardware.

## Organization
FBOSS SAI is organized into a few main components. They are summarized here, and
additional documentation can be found in the README.md file in the corresponding
directories.
* api: Libraries that wrap raw C SAI for additional safety. The main concepts
are SaiAttribute and SaiApi, which wrap the corresponding entities in SAI.
* store: Library that provides RAII wrappers around SAI objects, and
ref-counted storage for those objects. Relies on the api library.
* switch: Contains SaiSwitch -- the actual implementation of HwSwitch. Uses the
api and store libraries to handle usage of SAI.
* fake: An all software SAI implementation used for unit testing api and switch.
* impl: Build targets for linking in a particular SAI implementation.

## Reference
SAI: https://github.com/opencomputeproject/SAI
