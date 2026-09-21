# Set up FBOSS environment variables
cd /opt/fboss
source ./bin/setup_fboss_env

# Verify that all runtime dependencies are satisified for the test binary
# including the libraries installed in /opt/fboss:
ldd /opt/fboss/bin/$BINARY_NAME

# Ensure you dont see any 'not found'. Example:
# $ ldd /opt/fboss/bin/sai_test-fake
#         ...
#         # Good
#         libcurl.so.4 => /lib64/libcurl.so.4 (0x00007f3f26d39000)
#         libyaml-0.so.2 => /opt/fboss/lib/libyaml-0.so.2 (0x00007f3f26d18000)
#         ...
#         # Bad
#         libre2.so.9 => not found
#         libsodium.so.23 => not found
#         ...

# If there are any missing libraries, then those need to be installed on the
# switch using "sudo dnf install ..." if the switch has internet access.
# Alternatively, the missing libraries can be copied from the FBOSS build's
# scratch path `/opt/app/FBOSS_DIR/tmp_bld_dir/installed/<missing_lib*>/` to
# switch `/opt/fboss/lib/`.
