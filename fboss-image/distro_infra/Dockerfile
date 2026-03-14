FROM quay.io/centos/centos:stream9

RUN dnf install -y 'dnf-command(config-manager)' && \
    dnf config-manager --set-enabled crb && \
    dnf install -y epel-release epel-next-release && \
    dnf install -y --allowerasing \
    wget curl tcpdump zstd iputils which net-tools iproute \
    man dnsmasq vim nginx procps-ng && \
    dnf clean all && rm -rf /var/cache/dnf

RUN mkdir -p /distro_infra/dnsmasq_conf.d
COPY parts/run_distro_infra.sh /distro_infra
COPY parts/post_tftp.sh /distro_infra
COPY parts/ipxe/ipxev4.efi /distro_infra
COPY parts/ipxe/ipxev6.efi /distro_infra
COPY parts/autoexec.ipxe /distro_infra
COPY parts/nginx.conf /distro_infra

RUN mkdir -p /distro_infra/persistent
WORKDIR /distro_infra/persistent
