# On the container used for building FBOSS:

# Navigate to the fboss repository
cd /var/FBOSS/fboss

# Creates a package directory with prefix /var/FBOSS/tmp_bld_dir/fboss_bins
./fboss/oss/scripts/package-fboss.py --copy-root-libs --scratch-path /var/FBOSS/tmp_bld_dir/

# Creates a tarball called "fboss_bins.tar.zst" under /var/FBOSS/tmp_bld_dir/
./fboss/oss/scripts/package-fboss.py --copy-root-libs --scratch-path /var/FBOSS/tmp_bld_dir/ --compress
