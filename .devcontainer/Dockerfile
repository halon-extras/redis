FROM ubuntu:24.04
LABEL org.opencontainers.image.authors="support@halon.io"

ARG HALON_REPO_USER
ARG HALON_REPO_PASS

RUN apt-get update

RUN apt-get install -y wget gnupg
RUN apt-get install -y apt-transport-https

RUN wget -qO - https://raw.githubusercontent.com/halon/changelog/master/pgp-keys/7F0A73B5.asc | apt-key add -
RUN echo "deb https://repo.halon.io/ noble stable" >> /etc/apt/sources.list.d/halon.list
RUN echo "machine repo.halon.io login ${HALON_REPO_USER} password ${HALON_REPO_PASS}" >> /etc/apt/auth.conf
RUN apt-get update && apt-get install -y halon=6.3.0

RUN /usr/bin/install -d /var/run/halon
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH

RUN apt-get install -y build-essential cmake git libssl-dev curl

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

RUN ldconfig

RUN echo -n "UBU2404" > /OSRELEASE.txt

RUN apt-get install -y supervisor
COPY ./supervisord.conf /etc/supervisor/conf.d/supervisord.conf
CMD ["/usr/bin/supervisord"]