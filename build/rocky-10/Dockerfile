FROM rockylinux/rockylinux:10.1
LABEL org.opencontainers.image.authors="support@halon.io"

ARG TARGETARCH
ARG TARGETOS

ARG HALON_REPO_USER
ARG HALON_REPO_PASS

RUN dnf update -y && dnf install -y epel-release

RUN echo "[halon]" >> /etc/yum.repos.d/Halon.repo
RUN echo "name=Halon repository" >> /etc/yum.repos.d/Halon.repo
RUN echo "baseurl=https://${HALON_REPO_USER}:${HALON_REPO_PASS}@repo.halon.io/dists/rhel10/${TARGETARCH}/stable" >> /etc/yum.repos.d/Halon.repo
RUN echo "gpgcheck=1" >> /etc/yum.repos.d/Halon.repo
RUN echo "gpgkey=https://raw.githubusercontent.com/halon/pgp-keys/master/pgp-keys/7F0A73B5.asc" >> /etc/yum.repos.d/Halon.repo
RUN echo "enabled=1" >> /etc/yum.repos.d/Halon.repo
RUN dnf install -y halon-smtpd-6.11.0

RUN dnf install -y cmake make gcc-c++ rpm-build git openssl-devel

RUN mkdir /tmp/halon
RUN cd /tmp/halon \
    && curl -O -L https://github.com/redis/hiredis/archive/refs/tags/v1.4.1.tar.gz \
    && echo "ca3180359a8b1275838a45415851f8cd5c411e27bdbf18f4823012e45507d2e4 v1.4.1.tar.gz" | sha256sum -c \
    && tar -zxvf v1.4.1.tar.gz && cd hiredis-1.4.1 \
    && mkdir -p /hiredis && cp COPYING /hiredis/LICENSE \
    && mkdir build && cd build \
    && cmake -DENABLE_SSL=ON .. \
    && make && make install
RUN cd /tmp/halon \
    && curl -O -L https://github.com/sewenew/redis-plus-plus/archive/refs/tags/1.3.15.tar.gz \
    && echo "e9288fda74f1501c62dd9ab7d6a8dc67de51aa4d5b7c71770baa51987a945941 1.3.15.tar.gz" | sha256sum -c \
    && tar -zxvf 1.3.15.tar.gz && cd redis-plus-plus-1.3.15 \
    && mkdir -p /redis-plus-plus && cp LICENSE /redis-plus-plus/LICENSE \
    && mkdir build && cd build \
    && cmake -DREDIS_PLUS_PLUS_USE_TLS=ON .. \
    && make && make install
RUN rm -rf /tmp/halon

RUN echo -n "ROCKY10" > /OSRELEASE.txt

COPY build.sh /build.sh
CMD ["/build.sh"]
