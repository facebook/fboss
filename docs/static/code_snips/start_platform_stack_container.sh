# Use stable commits
rm -rf build/deps/github_hashes/
tar xvzf fboss/oss/stable_commits/latest_stable_hashes.tar.gz

# If you know your host has enough space for the build
sudo docker run -d \
    -it --name=FBOSS_DOCKER_CONTAINER \
    -v $PWD:/var/FBOSS/fboss:z \
    fboss_docker:latest bash

# A full FBOSS build may take significant space (>50GB of storage). You
# can mount a volume with more storage for building by using the -v flag
sudo docker run -d \
    -it --name=FBOSS_DOCKER_CONTAINER \
    -v $PWD:/var/FBOSS/fboss:z \
    -v /opt/app/localbuild:/var/FBOSS/tmp_bld_dir:z \
    fboss_docker:latest bash
