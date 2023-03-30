# Workflows

## Workflow: Building latest FBOSS

Follow steps in:
 - Building FBOSS OSS on Containers (but skip the step to pin to latest stable commit hashes)

## Workflow: Building latest stable FBOSS

Follow steps in:
 - Building FBOSS OSS on Containers (including the step to pin to latest stable commit hashes)

## Workflow: Iterate on FBOSS changes

For the incremental FBOSS OSS build for FBOSS changes –
 - Make the FBOSS code changes in “<scratch_path>/repos/github.com-facebook-fboss.git/”
 - Then, cd “<scratch_path>/build/fboss/” and 
 - Execute “./run_cmake.py --install”

## Workflow: Iterate on SAI lib changes 

### SAI lib libsai.a changes not involving SAI header changes

 - Build libsai.a incrementally as per instructions from ASIC vendor
 - cd <scratch_path>/installed/sai_impl->/
 - Replace libsai_impl.a with the newly built libsai.a
 - cd “<scratch_path>/build/fboss/” and 
 - Execute “./run_cmake.py --install”


### SAI lib libsai.a changes involving SAI header changes

 - Build libsai.a incrementally as per instructions from ASIC vendor
 - rm -rf /var/FBOSS/built-sai
 - mkdir -p /var/FBOSS/built-sai/experimental
 - cp /var/FBOSS/bcm_sai/output/$OUTPUT_DIR/libraries/libsai.a /var/FBOSS/built-sai/libsai_impl.a
 - cp /var/FBOSS/bcm_sai/include/experimental/*.* /var/FBOSS/built-sai/experimental
 - cd /var/FBOSS/fboss.git/installer/centos-8-x64_64
 - ./build-helper.py /var/FBOSS/built-sai/libsai_impl.a /var/FBOSS/built-sai/experimental/ /var/FBOSS/sai_impl_output
 - export SAI_BRCM_IMPL=1 // Optional
 - export GETDEPS_USE_WGET=1
 - cd /var/FBOSS/fboss.git
 - ./build/fbcode_builder/getdeps.py build --allow-system-packages --scratch-path /var/FBOSS/tmp_bld_dir fboss

## Workflow: Iterate on FBOSS changes for "--cmake-target" option

 - Make the FBOSS code changes in “<scratch_path>/repos/github.com-facebook-fboss.git/”
 - Then, rebuild FBOSS OSS using --cmake-target option -
```
 cd /var/FBOSS/fboss.git
 time ./build/fbcode_builder/getdeps.py build --allow-system-packages --scratch-path /var/FBOSS/tmp_bld_dir fboss --cmake-target sai_test-sai_impl-1.11.0"
```

## Workflow: Iterate on SAI lib changes for "--cmake-target" option

### SAI lib libsai.a changes not involving SAI header changes (FBOSS OSS built using "--cmake-target" option)

 - Build libsai.a incrementally as per instructions from ASIC vendor
 - cd <scratch_path>/installed/sai_impl->/
 - Replace libsai_impl.a with the newly built libsai.a
 - Then, rebuild FBOSS OSS using --cmake-target option -
```
 cd /var/FBOSS/fboss.git
 time ./build/fbcode_builder/getdeps.py build --allow-system-packages --scratch-path /var/FBOSS/tmp_bld_dir fboss --cmake-target sai_test-sai_impl-1.11.0"
```

### SAI lib libsai.a changes involving SAI header changes (FBOSS OSS built using "--cmake-target" option)

 - Build libsai.a incrementally as per instructions from ASIC vendor
 - rm -rf /var/FBOSS/built-sai
 - mkdir -p /var/FBOSS/built-sai/experimental
 - cp /var/FBOSS/bcm_sai/output/$OUTPUT_DIR/libraries/libsai.a /var/FBOSS/built-sai/libsai_impl.a
 - cp /var/FBOSS/bcm_sai/include/experimental/*.* /var/FBOSS/built-sai/experimental
 - cd /var/FBOSS/fboss.git/installer/centos-8-x64_64
 - ./build-helper.py /var/FBOSS/built-sai/libsai_impl.a /var/FBOSS/built-sai/experimental/ /var/FBOSS/sai_impl_output
 - export SAI_BRCM_IMPL=1 // Optional
 - export GETDEPS_USE_WGET=1
 - cd /var/FBOSS/fboss.git
 - ./build/fbcode_builder/getdeps.py build --allow-system-packages --scratch-path /var/FBOSS/tmp_bld_dir fboss

