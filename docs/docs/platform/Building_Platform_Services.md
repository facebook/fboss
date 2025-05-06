---
id: building_platform_services
title: Building Platform Services
keywords:
  - FBOSS
  - OSS
  - build
  - docker
oncall: fboss_oss
---

Refer to [Build Page](../build/Building_FBOSS_on_containers.md) for Build
Instructions

Building the entire fboss OSS repository could be time consuming. Optionally,
you can just build the platform services by running

```
time ./build/fbcode_builder/getdeps.py build --allow-system-packages \
--extra-cmake-defines='{"CMAKE_BUILD_TYPE": "MinSizeRel", "CMAKE_CXX_STANDARD": "20"}' \
--scratch-path /var/FBOSS/tmp_bld_dir --cmake-target fboss_platform_services fboss
```

You can also build a specific platform binary by changing the `--cmake_target`.
