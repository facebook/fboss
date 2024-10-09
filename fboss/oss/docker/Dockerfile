FROM quay.io/centos/centos:stream9

RUN dnf install git sudo lsof -y

# Use /var/FBOSS as the set location to clone the git repository and store the outputs of the build.
WORKDIR /var/FBOSS/

RUN mkdir -p /var/FBOSS/fboss
COPY . fboss
WORKDIR fboss

RUN rm -rf build/deps/github_hashes/
RUN tar xvzf fboss/oss/stable_commits/latest_stable_hashes.tar.gz

RUN dnf install -y 'dnf-command(config-manager)'
RUN dnf config-manager --set-enabled crb
RUN dnf install -y epel-release epel-next-release
RUN dnf install libcurl libcurl-devel -y --allowerasing
RUN ./build/fbcode_builder/getdeps.py install-system-deps --recursive fboss
RUN dnf install bison flex -y
RUN dnf group install "Development Tools" -y
RUN dnf install openssl* -y
RUN python3 -m pip install gitpython

