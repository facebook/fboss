# Overview
Build times have been one of the main pain points of FSDB binary and its core libraries. To improve build times, a few approaches were pursued. One of them was to explicitly instantiate costly templates. Under the directory fboss/fsdb/oper/instantiations/ we have defined the template instantiations of costly high level classes.

While this yielded good results, the instantiations themselves were in the critical path of the build (fsdb_cow_storage and fsdb_naive_periodic_subscribable_storage). To further improve build time, we took a look at the costly instantiations within these libraries.
The files in this sub directory contains struct and function level explicit instantiations of costly templates to improve the build time of these libraries.

## Purpose

These instantiations exist solely for the purpose of improving build times. They pre-compile commonly used template instantiations to reduce compilation overhead in dependent code.

## Note for Code Readers

If you are trying to understand the FSDB core logic, you can safely skip reading the code in this directory. These files do not contain core logic or schema definitions, but rather are optimization artifacts for the build system.
