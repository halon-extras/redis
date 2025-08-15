FROM --platform=linux/amd64 rockylinux:8
LABEL org.opencontainers.image.authors="support@halon.io"

ARG HALON_REPO_USER
ARG HALON_REPO_PASS

RUN yum update -y && yum install -y epel-release

RUN echo "[halon]" >> /etc/yum.repos.d/Halon.repo
RUN echo "name=Halon repository" >> /etc/yum.repos.d/Halon.repo
RUN echo "baseurl=https://${HALON_REPO_USER}:${HALON_REPO_PASS}@repo.halon.io/dists/rhel8/amd64/stable" >> /etc/yum.repos.d/Halon.repo
RUN echo "gpgcheck=1" >> /etc/yum.repos.d/Halon.repo
RUN echo "gpgkey=https://raw.githubusercontent.com/halon/changelog/master/pgp-keys/7F0A73B5.asc" >> /etc/yum.repos.d/Halon.repo
RUN echo "enabled=1" >> /etc/yum.repos.d/Halon.repo
RUN yum install -y halon-5.10.0

RUN yum install -y cmake3 make gcc-c++ rpm-build git openssl-devel

RUN mkdir /tmp/halon
RUN cd /tmp/halon \
    && curl -O -L https://github.com/redis/hiredis/archive/refs/tags/v1.1.0.tar.gz \
    && echo "fe6d21741ec7f3fc9df409d921f47dfc73a4d8ff64f4ac6f1d95f951bf7f53d6 v1.1.0.tar.gz" | sha256sum -c \
    && tar -zxvf v1.1.0.tar.gz && cd hiredis-1.1.0 \
    && mkdir -p /hiredis && cp COPYING /hiredis/LICENSE \
    && mkdir build && cd build \
    && cmake -DENABLE_SSL=ON .. \
    && make && make install
RUN cd /tmp/halon \
    && curl -O -L https://github.com/sewenew/redis-plus-plus/archive/refs/tags/1.3.9.tar.gz \
    && echo "f890bee2bf81e85619e8cdd1189b9249daddf1562a89e04e2b685ba4ca37e0f8 1.3.9.tar.gz" | sha256sum -c \
    && tar -zxvf 1.3.9.tar.gz && cd redis-plus-plus-1.3.9 \
    && mkdir -p /redis-plus-plus && cp LICENSE /redis-plus-plus/LICENSE \
    && mkdir build && cd build \
    && cmake -DREDIS_PLUS_PLUS_USE_TLS=ON .. \
    && make && make install
RUN rm -rf /tmp/halon

RUN echo -n "CENTOS8" > /OSRELEASE.txt

RUN yum install -y 'dnf-command(config-manager)'
RUN yum config-manager --set-enabled powertools

COPY build.sh /build.sh
CMD ["/build.sh"]
