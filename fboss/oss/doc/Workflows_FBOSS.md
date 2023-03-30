# Workflows

## Workflow: Building latest FBOSS for first time

Follow steps in:
 - Building FBOSS OSS on Containers (but skip the step to pin to latest stable commit hashes)

## Workflow: Building latest stable FBOSS for first time

Follow steps in:
 - Building FBOSS OSS on Containers (including the step to pin to latest stable commit hashes)

## Workflow: Building latest FBOSS after successful FBOSS OSS build

From container running on VM -

```
 cd /var/FBOSS/fboss.git
 # Remove the stable commit hash pinning if it was done earlier
 rm -rf build/deps/github_hashes/facebook
 rm -rf build/deps/github_hashes/facebookincubator

 git stash # This is to save the build-helper changes
 git pull
 git stash pop # This is to restore the build-helper changes

 # Optionally ping to latest stable commit hashes
 tar -xvf fboss/oss/stable_commits/latest_stable_hashes.tar.gz

 # Build FBOSS
 ./build/fbcode_builder/getdeps.py build --allow-system-packages --scratch-path /var/FBOSS/tmp_bld_dir fboss
```

## Workflow: Iterate on FBOSS changes

From container running on VM -

For the incremental FBOSS OSS build for FBOSS changes –

```
 # First build FBOSS OSS without any code changes by following any of the previous workflows
 # Make the FBOSS code changes in “<scratch_path>/repos/github.com-facebook-fboss.git/”
 cd <scratch_path>/build/fboss/
 ./run_cmake.py --install
```

## Workflow: Iterate on SAI lib changes

### SAI lib libsai.a changes not involving SAI header changes

From container running on VM -

```
 # Build libsai.a incrementally as per instructions from ASIC vendor
 cd <scratch_path>/installed/sai_impl-*>/
 # Replace libsai_impl.a with the newly built libsai.a
 cd <scratch_path>/build/fboss/
 ./run_cmake.py --install
```


### SAI lib libsai.a changes involving SAI header changes

From container running on VM -

```
 # Build libsai.a incrementally as per instructions from ASIC vendor
 rm -rf /var/FBOSS/built-sai
 mkdir -p /var/FBOSS/built-sai/experimental
 cp /var/FBOSS/bcm_sai/output/$OUTPUT_DIR/libraries/libsai.a /var/FBOSS/built-sai/libsai_impl.a
 cp /var/FBOSS/bcm_sai/include/experimental/*.* /var/FBOSS/built-sai/experimental
 cd /var/FBOSS/fboss.git/fboss/oss/scripts
 ./build-helper.py /var/FBOSS/built-sai/libsai_impl.a /var/FBOSS/built-sai/experimental/ /var/FBOSS/sai_impl_output
 export SAI_BRCM_IMPL=1 // Optional
 export GETDEPS_USE_WGET=1
 cd /var/FBOSS/fboss.git
 ./build/fbcode_builder/getdeps.py build --allow-system-packages --scratch-path /var/FBOSS/tmp_bld_dir fboss
```

## Workflow: Iterate on FBOSS changes for "--cmake-target" option

From container running on VM -

```
 # Make the FBOSS code changes in “<scratch_path>/repos/github.com-facebook-fboss.git/”
 cd /var/FBOSS/fboss.git
 time ./build/fbcode_builder/getdeps.py build --allow-system-packages --scratch-path /var/FBOSS/tmp_bld_dir fboss --cmake-target sai_test-sai_impl-1.11.0"
```

## Workflow: Iterate on SAI lib changes for "--cmake-target" option

### SAI lib libsai.a changes not involving SAI header changes (FBOSS OSS built using "--cmake-target" option)

From container running on VM -

```
 # Build libsai.a incrementally as per instructions from ASIC vendor
 cd <scratch_path>/installed/sai_impl-*>/
 # Replace libsai_impl.a with the newly built libsai.a
 cd /var/FBOSS/fboss.git
 time ./build/fbcode_builder/getdeps.py build --allow-system-packages --scratch-path /var/FBOSS/tmp_bld_dir fboss --cmake-target sai_test-sai_impl-1.11.0"
```

### SAI lib libsai.a changes involving SAI header changes (FBOSS OSS built using "--cmake-target" option)

From container running on VM -

```
 # Build libsai.a incrementally as per instructions from ASIC vendor
 rm -rf /var/FBOSS/built-sai
 mkdir -p /var/FBOSS/built-sai/experimental
 cp /var/FBOSS/bcm_sai/output/$OUTPUT_DIR/libraries/libsai.a /var/FBOSS/built-sai/libsai_impl.a
 cp /var/FBOSS/bcm_sai/include/experimental/*.* /var/FBOSS/built-sai/experimental
 cd /var/FBOSS/fboss.git/fboss/oss/scripts
 ./build-helper.py /var/FBOSS/built-sai/libsai_impl.a /var/FBOSS/built-sai/experimental/ /var/FBOSS/sai_impl_output
 export SAI_BRCM_IMPL=1 // Optional
 export GETDEPS_USE_WGET=1
 cd /var/FBOSS/fboss.git
 ./build/fbcode_builder/getdeps.py build --allow-system-packages --scratch-path /var/FBOSS/tmp_bld_dir fboss
```
